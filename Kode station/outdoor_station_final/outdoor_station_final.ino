#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BME680.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SEALEVELPRESSURE_HPA (1013.25f)
#define SCL_PIN D1
#define SDA_PIN D2
#define STATION_NAME "Outdoor Monitoring Station 01"

float CO_BASELINE = 0.0246;
float NO2_BASELINE = 0.05071;
float nh3_baseline = 0.000;
float voltageCO = 0;
float voltageNO2 = 0;

#define CO_SENSITIVITY 1.2
#define NO2_SENSITIVITY 2.0
#define NH3_SENSITIVITY 1.5

const char* ssid = "CALVIN-Student-2G";
const char* password = "CITStudentsOnly";
const char* mqtt_server = "10.251.255.73";

String mqtt_data_topic = "stations/" + String(STATION_NAME) + "/data";
String mqtt_info_topic = "stations/" + String(STATION_NAME) + "/information";
String mqtt_ping_topic = "stations/" + String(STATION_NAME) + "/ping";
String mqtt_pong_topic = "stations/" + String(STATION_NAME) + "/pong";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_ADS1115 ads;
Adafruit_BME680 bme;
const float S_analog = 32767.0;

unsigned long startTime;
bool setupPhase = true;
const unsigned long setupDuration = 5000;
const unsigned long messageDelay = 2000;
unsigned long lastMessageTime = 0;

bool adsConnected = false;
bool bmeConnected = false;

int co_zero_count = 0;
int no2_zero_count = 0;

void checkBMEConnection() {
  if (!bme.begin()) {
    Serial.println("BME680 disconnected!");
    bmeConnected = false;
  } else {
    bmeConnected = true;
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setGasHeater(320, 150);
  }
}

void sendSetupMessages() {
  client.publish(mqtt_data_topic.c_str(), "1");
  Serial.println("Sent '1' to data topic during setup phase");

  String setupJson = "{";
  setupJson += "\"BME680 Humidity\": [\"%\", \"Sensor Kelembaban\", [";
  setupJson += "[\"Kering\", 0, 30, \"F4A460\"],";
  setupJson += "[\"Nyaman\", 30, 60, \"90EE90\"],";
  setupJson += "[\"Lembap\", 60, 80, \"FFFF99\"],";
  setupJson += "[\"Sangat Lembap\", 80, 100, \"F4A460\"]";
  setupJson += "]],";

  setupJson += "\"BME680 Temperature\": [\"Celcius\", \"Sensor Suhu\", [";
  setupJson += "[\"Dingin\", -40, 18, \"ADD8E6\"],";
  setupJson += "[\"Nyaman\", 18, 27, \"90EE90\"],";
  setupJson += "[\"Hangat\", 27, 32, \"FFFF99\"],";
  setupJson += "[\"Panas\", 32, 85, \"F4A460\"]";
  setupJson += "]],";

  setupJson += "\"BME680 Pressure\": [\"hPa\", \"Sensor Tekanan\", [";
  setupJson += "[\"Rendah\", 300, 1000, \"F4A460\"],";
  setupJson += "[\"Normal\", 1000, 1020, \"90EE90\"],";
  setupJson += "[\"Tinggi\", 1020, 1100, \"FFFF99\"]";
  setupJson += "]],";

  setupJson += "\"BME680 VOC\": [\"kOhm\", \"Sensor Volatile Organic Compounds (VOC)\", [";
  setupJson += "[\"Berbahaya\", 0, 5, \"FF0000\"],";
  setupJson += "[\"Sangat Tidak Sehat\", 5, 10, \"FF6347\"],";
  setupJson += "[\"Tidak Sehat\", 10, 20, \"FFA07A\"],";
  setupJson += "[\"Sedang\", 20, 50, \"FFFF99\"],";
  setupJson += "[\"Baik\", 50, 300, \"90EE90\"]";
  setupJson += "]],";

  setupJson += "\"MICS6814 CO\": [\"ppm\", \"Sensor Karbon Monoksida (CO)\", [";
  setupJson += "[\"Baik\", 0, 4.4, \"D4FCA9\"],";
  setupJson += "[\"Sedang\", 4.4, 9.4, \"FCE4A9\"],";
  setupJson += "[\"Tidak Sehat untuk Sensitif\", 9.4, 12.4, \"FCA999\"],";
  setupJson += "[\"Tidak Sehat\", 12.4, 15.4, \"FC7F7F\"],";
  setupJson += "[\"Berbahaya\", 15.4, 20, \"FC4F4F\"]";
  setupJson += "]],";

  setupJson += "\"MICS6814 NO2\": [\"ppm\", \"Sensor Nitrogen Dioksida (NO2)\", [\n";
  setupJson += "  [\"Sangat Baik\", 0.000, 0.053, \"D4FCA9\"],\n";
  setupJson += "  [\"Baik\", 0.054, 0.300, \"FCE4A9\"],\n";
  setupJson += "  [\"Tidak Sehat\", 0.300, 0.649, \"FC7F7F\"],\n";
  setupJson += "  [\"Sangat Tidak Sehat\", 0.650, 1.249, \"FC4F4F\"],\n";
  setupJson += "  [\"Berbahaya\", 1.250, 2.049, \"FC0000\"]\n";
  setupJson += "]],";

  setupJson += "\"raindrop sensor\": [\"\", \"Sensor Intensitas Hujan\", [";
  setupJson += "[\"Tidak Hujan\", 13000, 20000, \"D4FCA9\"],";
  setupJson += "[\"Gerimis\", 7000, 13000, \"FCE4A9\"],";
  setupJson += "[\"Hujan\", 0, 7000, \"FCA999\"]";
  setupJson += "]]";
  setupJson += "}";

  client.publish(mqtt_info_topic.c_str(), setupJson.c_str());
  Serial.println("Sent setup JSON to information topic");
  Serial.println(setupJson);
}

void sendDataMessage() {
  checkBMEConnection();

  Wire.beginTransmission(0x48);
  adsConnected = (Wire.endTransmission() == 0);

  float co = 0, no2 = 0;
  int16_t raindrop_adc = 0;
  float ppmCO = 0, ppmNO2 = 0;
  String raindrop_status = "none";

  if (adsConnected) {
    int16_t rawCO = ads.readADC_SingleEnded(0);
    int16_t rawNO2 = ads.readADC_SingleEnded(1);
    raindrop_adc = ads.readADC_SingleEnded(2);

    voltageNO2 = rawNO2 * 0.125 / 1000.0;
    Serial.println(voltageCO);
    Serial.println(voltageNO2);

    ppmCO = calculateCO_PPM(voltageCO);
    ppmNO2 = calculateNO2_PPM(voltageNO2);

    if (raindrop_adc < 50 || raindrop_adc == 32767) {
      raindrop_status = "none";
    } else {
      raindrop_status = (raindrop_adc > 18000) ? "Dry" : (raindrop_adc < 7000) ? "Wet"
                                                                               : "Slightly Wet";
    }
  }

  if (voltageCO == 0) {
    co_zero_count++;
  } else {
    co_zero_count = 0;
  }

  if (voltageNO2 == 0) {
    no2_zero_count++;
  } else {
    no2_zero_count = 0;
  }

  if (co_zero_count >= 2) {
    co = no2 = -1;
  }

  float temperature = 0, humidity = 0, pressure = 0, gas = 0;
  if (bmeConnected && bme.performReading()) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    pressure = bme.pressure / 100.0;
    gas = (bme.gas_resistance / 1000.0);
  } else {
    bmeConnected = false;
  }

  String payload = "{";
  payload += "\"BME680 Humidity\": " + (bmeConnected ? String(humidity) : "\"none\"") + ",";
  payload += "\"BME680 Temperature\": " + (bmeConnected ? String(temperature) : "\"none\"") + ",";
  payload += "\"BME680 Pressure\": " + (bmeConnected ? String(pressure) : "\"none\"") + ",";
  payload += "\"BME680 VOC\": " + (bmeConnected ? String(gas) : "\"none\"") + ",";
  payload += "\"MICS6814 CO\": " + ((ppmNO2 > 2.5) ? "\"none\"" : String(ppmCO, 3)) + ",";
  payload += "\"MICS6814 NO2\": " + ((ppmNO2 > 2.5) ? "\"none\"" : String(ppmNO2, 5)) + ",";
  payload += "\"raindrop sensor\": " + ((raindrop_adc < 50) ? "\"none\"" : String(raindrop_adc));
  payload += "}";

  client.publish(mqtt_data_topic.c_str(), payload.c_str());
  Serial.println("Data published to MQTT!");
  Serial.println(payload);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String incomingTopic = String(topic);
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("Message arrived [");
  Serial.print(incomingTopic);
  Serial.print("]: ");
  Serial.println(message);

  if (incomingTopic == mqtt_ping_topic && message == "ping") {
    client.publish(mqtt_pong_topic.c_str(), "pong");
    Serial.println("Responded with 'pong'");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(STATION_NAME)) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(mqtt_ping_topic.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }


  Wire.begin(SDA_PIN, SCL_PIN);
  adsConnected = ads.begin(0x48);
  if (adsConnected) {
    ads.setGain(GAIN_ONE);
    Serial.println("ADS1115 initialized successfully");
  } else {
    Serial.println("Failed to initialize ADS1115");
  }
  checkBMEConnection();
  startTime = millis();
  Serial.println("Setup complete, starting setup phase (1 minute)");
}


float calculateCO_PPM(float sensorValue) {
  float ratio = sensorValue / CO_BASELINE;

  float ppm = 4.4 * pow(ratio, -1.179);

  if (ppm < 0.1) ppm = 0.1;
  if (ppm > 1000) ppm = 1000;  

  return ppm;
}

float calculateNO2_PPM(float sensorValue) {
  float ratio = sensorValue / NO2_BASELINE;

  float ppm = 0.64 * pow(ratio, 1.2);

  if (ppm < 0.001) ppm = 0.001;
  if (ppm > 10) ppm = 10;

  return ppm;
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Reconnecting to MQTT...");
    if (client.connect(STATION_NAME)) {
      client.subscribe(mqtt_ping_topic.c_str());
      Serial.println("Reconnected to MQTT Broker!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  client.loop();
  if (millis() - lastMessageTime >= messageDelay) {
    if (setupPhase && millis() - startTime < setupDuration) {
      sendSetupMessages();
    } else {
      setupPhase = false;
      sendDataMessage();
    }
    lastMessageTime = millis();
  }
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}
