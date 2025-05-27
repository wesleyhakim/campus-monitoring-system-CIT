#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define STATION_NAME "Water Station 01"

const char* ssid = "CALVIN-Student-2G";
const char* password = "CITStudentsOnly";
const char* mqtt_server = "10.251.255.73";

String mqtt_data_topic = "stations/" + String(STATION_NAME) + "/data";
String mqtt_info_topic = "stations/" + String(STATION_NAME) + "/information";
String mqtt_ping_topic = "stations/" + String(STATION_NAME) + "/ping";
String mqtt_pong_topic = "stations/" + String(STATION_NAME) + "/pong";

WiFiClient espClient;
PubSubClient client(espClient);

#define SENSOR D2

unsigned long startTime;
bool setupPhase = true;
const unsigned long setupDuration = 5000;
const unsigned long messageDelay = 2000;
unsigned long lastMessageTime = 0;
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;

unsigned long lastPulseTime = 0;
const unsigned long sensorTimeout = 5000;
unsigned long lastSensorCheckTime = 0;
const unsigned long sensorCheckInterval = 1000;

void IRAM_ATTR pulseCounter() {
    pulseCount++;
    lastPulseTime = millis();
}

void setup() {
    Serial.begin(115200);
    delay(10);

    pinMode(SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

    pulseCount = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    previousMillis = 0;
    lastPulseTime = millis();

    Serial.println("Water Flow Meter - Serial Output");
    Serial.println("-------------------------------");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    
    connectToMQTT();
    
    startTime = millis();
    lastMessageTime = 0;
    lastSensorCheckTime = millis();
}

void connectToMQTT() {
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

void sendSetupMessages() {
    client.publish(mqtt_data_topic.c_str(), "1");
    Serial.println("Sent '1' to data topic during setup phase");
    
    String setupJson = "{";
    setupJson += "\"YF-S201 Flow rate\": [\"L/min\", \"Sensor Aliran Air\", [";
    setupJson += "[\"Tidak Ada Aliran\", 0, 0.5, \"FC4F4F\"],";
    setupJson += "[\"Sangat Rendah\", 0.51, 1.5, \"FC7F7F\"],";
    setupJson += "[\"Rendah\", 1.51, 3.0, \"FCA999\"],";
    setupJson += "[\"Sedang\", 3.01, 6.0, \"FCE4A9\"],";
    setupJson += "[\"Tinggi\", 6.01, 20.0, \"D4FCA9\"]";
    setupJson += "]],";

    setupJson += "\"YF-S201 Total rate mL\": [\"mL\", \"Sensor Total Aliran Air (mL)\", [";
    setupJson += "[\"none\"]";
    setupJson += "]],";

    setupJson += "\"YF-S201 Total rate L\": [\"L\", \"Sensor Total Aliran Air (L)\", [";
    setupJson += "[\"none\"]";
    setupJson += "]]";
    
    setupJson += "}";
    
    client.publish(mqtt_info_topic.c_str(), setupJson.c_str());
    Serial.println("Sent setup JSON to information topic");
    Serial.println(setupJson);
}

void sendDataMessage() {
    int state = digitalRead(D2);
    Serial.println(state);
    bool sensorConnected = (state == HIGH);

    String dataJson = "{";
    
    if (sensorConnected) {
        dataJson += "\"YF-S201 Flow rate\":" + String(flowRate) + ",";
        dataJson += "\"YF-S201 Total rate mL\":" + String(totalMilliLitres) + ",";
        dataJson += "\"YF-S201 Total rate L\":" + String(totalLitres);
    } else {
        dataJson += "\"YF-S201 Flow rate\":\"none\",";
        dataJson += "\"YF-S201 Total rate mL\":\"none\",";
        dataJson += "\"YF-S201 Total rate L\":\"none\"";
    }
    
    dataJson += "}";
    
    client.publish(mqtt_data_topic.c_str(), dataJson.c_str());
    Serial.println("Sent data JSON to data topic");
    Serial.println(dataJson);
}

void updateFlowCalculations() {
    currentMillis = millis();
    if (currentMillis - previousMillis > interval) {
        pulse1Sec = pulseCount;
        pulseCount = 0;
 
            flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
            flowMilliLitres = (flowRate / 60) * 1000;
            flowLitres = (flowRate / 60);
            totalMilliLitres += flowMilliLitres;
            totalLitres += flowLitres;


        previousMillis = millis();
    }
}

void loop() {
    if (!client.connected()) {
        connectToMQTT();
    }
    client.loop();

    updateFlowCalculations();
    
    unsigned long currentTime = millis();
    
    if (setupPhase) {
        if (currentTime - lastMessageTime >= messageDelay) {
            sendSetupMessages();
            lastMessageTime = currentTime;
        }
        
        if (currentTime - startTime >= setupDuration) {
            setupPhase = false;
            Serial.println("Setup phase complete. Switching to normal operation.");
            lastMessageTime = currentTime;
        }
    } else {
        if (currentTime - lastMessageTime >= messageDelay) {
            sendDataMessage();
            lastMessageTime = currentTime;
        }
    }
}