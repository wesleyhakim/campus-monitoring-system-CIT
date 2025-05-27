#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <BH1750.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define STATION_NAME "Indoor Monitoring Station 01"  

#define PMS_RX 14
#define PMS_TX 12

const char* ssid = "CALVIN-Student-2G"; 
const char* password = "CITStudentsOnly";
const char* mqtt_server = "10.251.255.73";

String mqtt_data_topic = "stations/" + String(STATION_NAME) + "/data";
String mqtt_info_topic = "stations/" + String(STATION_NAME) + "/information";
String mqtt_ping_topic = "stations/" + String(STATION_NAME) + "/ping";
String mqtt_pong_topic = "stations/" + String(STATION_NAME) + "/pong";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BME680 bme;
BH1750 lightMeter;
SoftwareSerial pmsSerial(PMS_RX, PMS_TX);

unsigned long startTime;
bool setupPhase = true;
const unsigned long setupDuration = 5000;
const unsigned long messageDelay = 2000;
unsigned long lastMessageTime = 0;

bool bmeConnected = false;
bool lightAvailable = false;
bool pmsAvailable = false;

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
    
    Wire.begin(D2, D1);
    
    checkBMEConnection();
    if (bmeConnected) {
        Serial.println("BME680 sensor initialized successfully!");
    } else {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
    }
    
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        lightAvailable = true;
        Serial.println("BH1750 sensor initialized successfully!");
    } else {
        Serial.println("Could not find BH1750 sensor!");
        lightAvailable = false;
    }
    
    pmsSerial.begin(9600);
    pmsAvailable = true;
    
    startTime = millis();
    lastMessageTime = startTime;
}

void loop() {
    unsigned long currentTime = millis();
    
    if (setupPhase && currentTime - startTime >= setupDuration) {
        setupPhase = false;
        Serial.println("Setup phase completed. Switching to regular data transmission.");
    }
    
    if (currentTime - lastMessageTime >= messageDelay) {
        if (setupPhase) {
            sendSetupMessages();
        } else {
            sendSensorData();
        }
        
        lastMessageTime = currentTime;
    }
    
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
}

void sendSetupMessages() {
    client.publish(mqtt_data_topic.c_str(), "1");
    Serial.println("Sent setup signal '1' to: " + mqtt_data_topic);

    String infoPayload = "{";

    infoPayload += "\"BH1750 Light\": [\"Lux\", \"Sensor Cahaya\", [";
    infoPayload += "[\"Gelap\", 0, 50, \"FFA07A\"],";
    infoPayload += "[\"Sedikit Terang\", 50, 100, \"FFFACD\"],";
    infoPayload += "[\"Terang\", 100, 1400, \"87CEFA\"]";
    infoPayload += "]],";

    infoPayload += "\"PMS5003 PM1\": [\"µg/m³\", \"Sensor Debu PM 1.0\", [";
    infoPayload += "[\"Baik\", 0, 12, \"90EE90\"],";
    infoPayload += "[\"Sedang\", 12, 35, \"FFFF99\"],";
    infoPayload += "[\"Tidak Sehat\", 35, 55, \"FFA07A\"],";
    infoPayload += "[\"Sangat Tidak Sehat\", 55, 150, \"FF6347\"]";
    infoPayload += "]],";

    infoPayload += "\"PMS5003 PM2.5\": [\"µg/m³\", \"Sensor Debu PM 2.5\", [";
    infoPayload += "[\"Baik\", 0, 12, \"90EE90\"],";
    infoPayload += "[\"Sedang\", 12, 35, \"FFFF99\"],";
    infoPayload += "[\"Tidak Sehat\", 35, 55, \"FFA07A\"],";
    infoPayload += "[\"Sangat Tidak Sehat\", 55, 150, \"FF6347\"]";
    infoPayload += "]],";

    infoPayload += "\"PMS5003 PM10\": [\"µg/m³\", \"Sensor Debu PM 10.0\", [";
    infoPayload += "[\"Baik\", 0, 54, \"90EE90\"],";
    infoPayload += "[\"Sedang\", 55, 154, \"FFFF99\"],";
    infoPayload += "[\"Tidak Sehat untuk Sensitif\", 155, 254, \"FFD700\"],";
    infoPayload += "[\"Tidak Sehat\", 255, 354, \"FFA07A\"],";
    infoPayload += "[\"Sangat Tidak Sehat\", 355, 424, \"FF6347\"]";
    infoPayload += "]],";

    infoPayload += "\"BME680 Humidity\": [\"%\", \"Sensor Kelembaban\", [";
    infoPayload += "[\"Kering\", 0, 30, \"F4A460\"],";
    infoPayload += "[\"Nyaman\", 30, 60, \"90EE90\"],";
    infoPayload += "[\"Lembap\", 60, 80, \"FFFF99\"],";
    infoPayload += "[\"Sangat Lembap\", 80, 100, \"F4A460\"]";
    infoPayload += "]],";

    infoPayload += "\"BME680 Temperature\": [\"Celcius\", \"Sensor Suhu\", [";
    infoPayload += "[\"Dingin\", -40, 17, \"ADD8E6\"],";
    infoPayload += "[\"Nyaman\", 17, 28, \"90EE90\"],";
    infoPayload += "[\"Hangat\", 28, 32, \"FFFF99\"],";
    infoPayload += "[\"Panas\", 32, 85, \"F4A460\"]";
    infoPayload += "]],";

    infoPayload += "\"BME680 Pressure\": [\"hPa\", \"Sensor Tekanan\", [";
    infoPayload += "[\"Rendah\", 300, 1000, \"F4A460\"],";
    infoPayload += "[\"Normal\", 1000, 1020, \"90EE90\"],";
    infoPayload += "[\"Tinggi\", 1020, 1100, \"FFFF99\"]";
    infoPayload += "]],";

    infoPayload += "\"BME680 VOC\": [\"kOhm\", \"Sensor Volatile Organic Compounds (VOC)\", [";
    infoPayload += "[\"Berbahaya\", 0, 5, \"FF0000\"],";
    infoPayload += "[\"Sangat Tidak Sehat\", 5, 10, \"FF6347\"],";
    infoPayload += "[\"Tidak Sehat\", 10, 20, \"FFA07A\"],";
    infoPayload += "[\"Sedang\", 20, 50, \"FFFF99\"],";
    infoPayload += "[\"Baik\", 50, 300, \"90EE90\"]";
    infoPayload += "]]";

    infoPayload += "}";

    if (client.connected()) {
        client.publish(mqtt_info_topic.c_str(), infoPayload.c_str());
        Serial.println("Sent setup information to: " + mqtt_info_topic);
    } else {
        Serial.println("Failed to send info - MQTT disconnected");
    }
}

void sendSensorData() {
    checkBMEConnection();
    
    float temperature = -999;
    float humidity = -999;
    float pressure = -999;
    float gas = -999;
    float lux = -999;
    int pm1_0 = -999;
    int pm2_5 = -999;
    int pm10 = -999;
    
    bool tempReadSuccess = false;
    bool humidReadSuccess = false;
    bool pressReadSuccess = false;
    bool gasReadSuccess = false;
    bool luxReadSuccess = false;
    bool pm1ReadSuccess = false;
    bool pm2_5ReadSuccess = false;
    bool pm10ReadSuccess = false;
    
    if (bmeConnected && bme.performReading()) {
        temperature = bme.temperature;
        humidity = bme.humidity;
        pressure = bme.pressure / 100.0;
        gas = bme.gas_resistance / 1000.0;
        
        tempReadSuccess = (temperature > -50 && temperature < 100);
        humidReadSuccess = (humidity >= 0 && humidity <= 100);
        pressReadSuccess = (pressure > 300 && pressure < 1200);
        gasReadSuccess = (gas > 0);
        
        Serial.print("Temperature: "); 
        if (tempReadSuccess) {
            Serial.print(temperature); Serial.println(" C");
        } else {
            Serial.println("Invalid reading");
        }
        
        Serial.print("Humidity: "); 
        if (humidReadSuccess) {
            Serial.print(humidity); Serial.println(" %");
        } else {
            Serial.println("Invalid reading");
        }
        
        Serial.print("Pressure: "); 
        if (pressReadSuccess) {
            Serial.print(pressure); Serial.println(" hPa");
        } else {
            Serial.println("Invalid reading");
        }
        
        Serial.print("Gas: "); 
        if (gasReadSuccess) {
            Serial.print(gas); Serial.println(" kOhm");
        } else {
            Serial.println("Invalid reading");
        }
    } else {
        Serial.println("Failed to read from BME680 sensor");
        bmeConnected = false;
    }
    
    if (!lightAvailable || !lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        lightAvailable = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
        if (lightAvailable) {
            Serial.println("BH1750 sensor reconnected!");
        } else {
            Serial.println("BH1750 sensor not available");
        }
    }
    
    if (lightAvailable) {
        lux = lightMeter.readLightLevel();
        
        luxReadSuccess = (lux > 0);
        
        Serial.print("Light: ");
        if (luxReadSuccess) {
            Serial.print(lux); Serial.println(" lx");
        } else {
            Serial.println("Invalid reading");
            lightAvailable = false;
        }
    }
    
    if (pmsAvailable) {
        bool pmsDataAvailable = readPMS(&pm1_0, &pm2_5, &pm10);
        
        pm1ReadSuccess = pmsDataAvailable && (pm1_0 >= 0);
        pm2_5ReadSuccess = pmsDataAvailable && (pm2_5 >= 0);
        pm10ReadSuccess = pmsDataAvailable && (pm10 >= 0);
        
        if (pmsDataAvailable) {
            Serial.print("PM1.0: "); 
            if (pm1ReadSuccess) {
                Serial.print(pm1_0); Serial.println(" ug/m3");
            } else {
                Serial.println("Invalid reading");
            }
            
            Serial.print("PM2.5: "); 
            if (pm2_5ReadSuccess) {
                Serial.print(pm2_5); Serial.println(" ug/m3");
            } else {
                Serial.println("Invalid reading");
            }
            
            Serial.print("PM10: "); 
            if (pm10ReadSuccess) {
                Serial.print(pm10); Serial.println(" ug/m3");
            } else {
                Serial.println("Invalid reading");
            }
        } else {
            Serial.println("No PMS data available");
        }
    } else {
        Serial.println("PMS sensor not available");
    }
    
    String payload = "{";
    
    payload += "\"BH1750 Light\": ";
    payload += (lightAvailable && luxReadSuccess) ? String(lux) : "\"none\"";
    payload += ", ";
    
    payload += "\"PMS5003 PM1\": ";
    payload += (pmsAvailable && pm1ReadSuccess) ? String(pm1_0) : "\"none\"";
    payload += ", ";
    
    payload += "\"PMS5003 PM2.5\": ";
    payload += (pmsAvailable && pm2_5ReadSuccess) ? String(pm2_5) : "\"none\"";
    payload += ", ";
    
    payload += "\"PMS5003 PM10\": ";
    payload += (pmsAvailable && pm10ReadSuccess) ? String(pm10) : "\"none\"";
    payload += ", ";
    
    payload += "\"BME680 Humidity\": ";
    payload += (bmeConnected && humidReadSuccess) ? String(humidity) : "\"none\"";
    payload += ", ";
    
    payload += "\"BME680 Temperature\": ";
    payload += (bmeConnected && tempReadSuccess) ? String(temperature) : "\"none\"";
    payload += ", ";
    
    payload += "\"BME680 Pressure\": ";
    payload += (bmeConnected && pressReadSuccess) ? String(pressure) : "\"none\"";
    payload += ", ";
    
    payload += "\"BME680 VOC\": ";
    payload += (bmeConnected && gasReadSuccess) ? String(gas) : "\"none\"";
    
    payload += "}";

    if (payload.length() > MQTT_MAX_PACKET_SIZE - 50) {
        Serial.println("WARNING: Data payload may be too large for MQTT buffer!");
    }

    bool publishSuccess = client.publish(mqtt_data_topic.c_str(), payload.c_str());
    if (publishSuccess) {
        Serial.println("Data published to: " + mqtt_data_topic);
    } else {
        Serial.println("Failed to publish data. MQTT issue or payload too large.");
    }
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

bool readPMS(int* pm1_0, int* pm2_5, int* pm10) {
    if (pmsSerial.available()) {
        byte buffer[32];
        int bytesRead = pmsSerial.readBytes(buffer, 32);
        
        if (bytesRead == 32 && buffer[0] == 0x42 && buffer[1] == 0x4d) {
            *pm1_0 = (buffer[10] << 8) | buffer[11];
            *pm2_5 = (buffer[12] << 8) | buffer[13];
            *pm10 = (buffer[14] << 8) | buffer[15];
            return true;
        }
        return false;
    }
    return false;
}