//
// Created by Andri Yadi on 10/29/16.
//

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>
#include <NtpClientLib.h>

const char *AP_SSID = "[YOUR_SSID_NAME]";
const char *AP_PASS = "[YOUR_SSID_PASS]";

// Azure IoT Hub Settings --> CHANGE THESE
#define IOTHUB_HOSTNAME         "[YOUR_IOTHUB_NAME].azure-devices.net"
#define DEVICE_ID               "[YOUR_DEVICE_ID]"
#define DEVICE_KEY              "[YOUR_DEVICE_KEY]" //Primary key of the device

#define USE_BMP180              1

#define BUFFER_SIZE 100

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);

#if USE_BMP180
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;
#endif

const int LED_PIN = 15; //Pin to turn on/of LED a command from IoT Hub
unsigned long lastMillis = 0;

void connect(); // <- predefine connect() for setup()

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

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

//    static WiFiEventHandler e1, e2;

#if USE_BMP180
    if (bmp.begin()) {
        Serial.println("BMP INIT SUCCESS");
    }
#endif

//    WiFi.onEvent([](WiFiEvent_t e) {
//        Serial.printf("Event wifi -----> %d\n", e);
//    });
//    e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
//    e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

    NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
        if (ntpEvent) {
            Serial.print("Time Sync error: ");
            if (ntpEvent == noResponse)
                Serial.println("NTP server not reachable");
            else if (ntpEvent == invalidAddress)
                Serial.println("Invalid NTP server address");
        }
        else {
            Serial.print("Got NTP time: ");
            Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
        }
    });

    connect();
}

void onMessageCallback(const MQTT::Publish& msg) {

    if (msg.payload_len() == 0) {
        return;
    }

    //Serial.println(msg.payload_string());

    //Parse message JSON
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject((char*)msg.payload(), 3);
    if (json.success()) {
        if (strcmp(json["Name"].asString(), "ActivateRelay") >= 0) {
            auto params = json["Parameters"];
            auto isAct = (params["Activated"]);
            if (isAct) {
                Serial.println("Activated true");
                digitalWrite(LED_PIN, HIGH);
            }
            else {
                Serial.println("Activated false");
                digitalWrite(LED_PIN, LOW);
            }
        }
        else {
            Serial.print("Command name: ");
            Serial.println(json["Name"].asString());
        }
    }

}

void connect() {

    Serial.print("Connecting to WiFi...");
    WiFi.begin(AP_SSID, AP_PASS);

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
        client.onMessage(onMessageCallback);

        //client.subscribe(MQTT::Subscribe().add_topic(MQTT_SUBSCRIBE_TOPIC));

    } else {
        Serial.println("Could not connect to MQTT");
    }

    NTP.begin("pool.ntp.org", 1, true);
    NTP.setInterval(63);
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
        if(millis() - lastMillis > 3000 && timeStatus() != timeNotSet) {
            lastMillis = millis();

            //Read the actual temperature from sensor
            float temp, press;
            readSensor(&temp, &press);

            time_t currentTime = now();

//            String payload = "{\"DeviceId\":\"" + String(DEVICE_ID) + "\", \"MTemperature\":" + String(temp) + ", \"EventTime\":" + String(currentTime) + "}";
//            Serial.println(payload);
//
//            //client.publish(MQTT::Publish("devices/" + String(DEVICE_ID) + "/messages/events/", payload).set_qos(1));
//            client.publishToDefaultTopic(payload);

            AzureIoTHubMQTTClient::JsonKeyValueMap keyVal = {{"MTemperature", temp}, {"DeviceId", DEVICE_ID}, {"EventTime", currentTime}};

            client.sendEventWithKeyVal(keyVal);
        }
    }
    else {
        connect();
        return;
    }

    delay(10); // <- fixes some issues with WiFi stability
}
