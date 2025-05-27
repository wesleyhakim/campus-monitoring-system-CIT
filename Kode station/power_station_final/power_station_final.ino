#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <PZEM004Tv30.h>

#define STATION_NAME "Energy Station 01"

const char* ssid = "CALVIN-Student-2G";
const char* password = "CITStudentsOnly";
const char* mqtt_server = "10.251.255.73";
                       
WiFiClient espClient;
PubSubClient client(espClient);

PZEM004Tv30 pzem1(14, 12);

String mqtt_ping_topic = "stations/" + String(STATION_NAME) + "/ping";
String mqtt_pong_topic = "stations/" + String(STATION_NAME) + "/pong";

unsigned long startTime;
bool setupPhase = true;
const unsigned long setupDuration = 5000;
const unsigned long messageDelay = 2000;
unsigned long lastMessageTime = 0;

void reconnectMQTT() {
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

void setup() {
    Serial.begin(115200);
    delay(10);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    reconnectMQTT();
    
    if (pzem1.resetEnergy()) {
        Serial.println("Energy counter reset successfully.");
    } else {
        Serial.println("Failed to reset energy counter.");
    }

    startTime = millis();
}


void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    unsigned long currentMillis = millis();

    if (currentMillis - lastMessageTime >= messageDelay) {
        lastMessageTime = currentMillis;
        
        String dataTopic = "stations/" + String(STATION_NAME) + "/data";
        String infoTopic = "stations/" + String(STATION_NAME) + "/information";
        
        if (currentMillis - startTime < setupDuration) {
            client.publish(dataTopic.c_str(), "1");
            Serial.println("Sent '1' to data topic");
            
              String setupPayload = "{";
              setupPayload += "\"PZEM004T Voltage\": [\"V\", \"Sensor Tegangan\", [";
              setupPayload += "[\"Sangat Rendah\", 0, 180, \"FC4F4F\"],";
              setupPayload += "[\"Rendah\", 181, 210, \"FC7F7F\"],";
              setupPayload += "[\"Normal\", 211, 240, \"D4FCA9\"],";
              setupPayload += "[\"Tinggi\", 241, 260, \"FCE4A9\"],";
              setupPayload += "[\"Berbahaya\", 261, 300, \"FCA999\"]";
              setupPayload += "]],";

              setupPayload += "\"PZEM004T Current\": [\"A\", \"Sensor Arus Listrik\", [";
              setupPayload += "[\"Tidak Ada Arus\", 0, 0.1, \"FC4F4F\"],";
              setupPayload += "[\"Rendah\", 0,1, 3.0, \"FCA999\"],";
              setupPayload += "[\"Sedang\", 3.0, 5.0, \"FCE4A9\"],";
              setupPayload += "[\"Tinggi\", 5.0, 15.0, \"D4FCA9\"]";
              setupPayload += "]],";

              setupPayload += "\"PZEM004T Energy\": [\"Wh\", \"Sensor Total Konsumsi Energi\", [";
              setupPayload += "[\"none\"]";
              setupPayload += "]],";

              setupPayload += "\"PZEM004T Power Factor\": [\"PF\", \"Sensor Efisiensi Energi\", [";
              setupPayload += "[\"Sangat Buruk\", 0.00, 0.3, \"FC4F4F\"],";
              setupPayload += "[\"Buruk\", 0.31, 0.6, \"FC7F7F\"],";
              setupPayload += "[\"Cukup\", 0.61, 0.85, \"FCE4A9\"],";
              setupPayload += "[\"Baik\", 0.86, 0.95, \"D4FCA9\"],";
              setupPayload += "[\"Sangat Baik\", 0.96, 1.00, \"90EE90\"]";
              setupPayload += "]],";

              setupPayload += "\"PZEM004T Active Power\": [\"W\", \"Sensor Daya\", [";
              setupPayload += "[\"Tidak Ada Daya\", 0, 0.1, \"FC4F4F\"],";
              setupPayload += "[\"Rendah\", 0.1, 50, \"FC7F7F\"],";
              setupPayload += "[\"Sedang\", 51, 200, \"FCA999\"],";
              setupPayload += "[\"Tinggi\", 201, 300, \"FCE4A9\"],";
              setupPayload += "[\"Sangat Tinggi\", 300, 400, \"D4FCA9\"]";
              setupPayload += "]]";
              setupPayload += "}";

            client.publish(infoTopic.c_str(), setupPayload.c_str());
            Serial.println("Sent setup message to information topic");
        } else {
            float voltage1 = pzem1.voltage();
            float current1 = pzem1.current();
            float power1 = pzem1.power();
            float energy1 = pzem1.energy();
            float pf1 = pzem1.pf();

            String dataPayload = "{";
            dataPayload += "\"PZEM004T Voltage\": " + ((isnan(voltage1) || voltage1 < 10.0) ? "\"none\"" : String(voltage1, 2)) + ", ";
            dataPayload += "\"PZEM004T Current\": " + ((isnan(voltage1) || voltage1 < 10.0) ? "\"none\"" : String(current1, 2)) + ", ";
            dataPayload += "\"PZEM004T Energy\": " + ((isnan(voltage1) || voltage1 < 10.0) ? "\"none\"" : String(energy1, 2)) + ", ";
            dataPayload += "\"PZEM004T Power Factor\": " + ((isnan(voltage1) || voltage1 < 10.0) ? "\"none\"" : String(pf1, 2)) + ", ";
            dataPayload += "\"PZEM004T Active Power\": " + ((isnan(voltage1) || voltage1 < 10.0) ? "\"none\"" : String(power1, 2));
            dataPayload += "}";

            
            client.publish(dataTopic.c_str(), dataPayload.c_str());
            Serial.println("Sent data message to data topic");
            Serial.println(dataPayload);
        }
    }
}
