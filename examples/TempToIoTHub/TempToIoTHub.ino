//
// Created by Andri Yadi on 10/29/16.
//

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "Adafruit_BMP085.h"
#include "AzureIoTHubMQTTClient.h"

const char *AP_SSID = "Andromax-M3Y-C634";
const char *AP_PASS = "p@ssw0rd";

// Azure IoT Hub Settings
#define IOTHUB_HOSTNAME         "TesterIoTHub.azure-devices.net"
#define DEVICE_ID               "espectro-01"
#define DEVICE_KEY              "0qLLMH6FD6oh1HaUHr2wNMsDuSBBiZIiPGwYHe0/ZAs="

#define USE_BMP180              1

#define BUFFER_SIZE 100

const char* TARGET_URL = "/devices/";

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);

#if USE_BMP180
Adafruit_BMP085 bmp;
#endif

unsigned long lastMillis = 0;
unsigned long startMilis = 1477725780;

void connectMqtt(); // <- predefine connect() for setup()

//void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
//    Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
//    connectMqtt();
//}
//
//void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
//    Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
//    Serial.printf("Reason: %d\n", event_info.reason);
//}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);

//    static WiFiEventHandler e1, e2;

#if USE_BMP180
    if (bmp.begin()) {
        Serial.println("BMP INIT SUCCESS");
    }
#endif

    WiFi.begin(AP_SSID, AP_PASS);

//    WiFi.onEvent([](WiFiEvent_t e) {
//        Serial.printf("Event wifi -----> %d\n", e);
//    });
//    e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
//    e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

    connectMqtt();
}

void mqttCallback(const MQTT::Publish& pub) {

    Serial.print(pub.topic());
    Serial.println(" => ");
    if (pub.has_stream()) {
        uint8_t buf[BUFFER_SIZE];
        int read;
        while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
            Serial.write(buf, read);
        }
        pub.payload_stream()->stop();
        Serial.println("");
    } else {
        Serial.println(pub.payload_string());
    }
}

void connectMqtt() {
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.print("\nConnecting to MQTT...");
//    while (!client.connect(DEVICE_ID, MQTT_USERNAME, mqttPassword.c_str())) {
//        Serial.print(".");
//        delay(500);
//    }

    if (client.connect()) {

        Serial.println("Connected to MQTT");
        client.set_callback(mqttCallback);

        //client.subscribe(MQTT::Subscribe().add_topic(MQTT_SUBSCRIBE_TOPIC));

    } else {
        Serial.println("Could not connect to MQTT");
    }
}

void readSensor(float *temp, float *press) {

#if USE_BMP180
    *temp = bmp.readTemperature();
    *press = 1.0f*bmp.readPressure()/1000; //--> kilo
#else
    //If you don't have the sensor
    *temp = 20 + (rand() % 10 + 2);
    *press = 90 + (rand() % 8 + 2);
#endif

}

void loop() {
    if (client.connected()) {
        client.loop();

        // publish a message roughly every 3 second.
        if(millis() - lastMillis > 3000) {
            lastMillis = millis();

            //Read the actual temperature from sensor
            float temp, press;
            readSensor(&temp, &press);

            startMilis += 3; //TODO: get actual timestamp
            String payload = "{\"MTemperature\":" + String(temp) + ", \"EventTime\":" + String(startMilis) + "}";
            Serial.println(payload);

            //client.publish(MQTT::Publish("devices/" + String(DEVICE_ID) + "/messages/events/", payload).set_qos(1));
            client.publishToDefaultTopic(payload);
        }
    }
    else {
        connectMqtt();
        return;
    }

    delay(10); // <- fixes some issues with WiFi stability
}
