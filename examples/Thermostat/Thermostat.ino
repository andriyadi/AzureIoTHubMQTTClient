//
// Created by Andri Yadi on 10/29/16.
//

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>

const char *AP_SSID = "[YOUR_SSID_NAME]";
const char *AP_PASS = "[YOUR_SSID_PASS]";

// Azure IoT Hub Settings --> CHANGE THESE
#define IOTHUB_HOSTNAME         "[YOUR_IOTHUB_NAME].azure-devices.net"
#define DEVICE_ID               "[YOUR_DEVICE_ID]"
#define DEVICE_KEY              "[YOUR_DEVICE_KEY]" //Primary key of the device

#define USE_BMP180              1 //Set this to 0 if you don't have the sensor and generate random sensor value to publish

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);
WiFiEventHandler  e1, e2;

#if USE_BMP180
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;
#endif

const int LED_PIN = 15; //Pin to turn on/of LED a command from IoT Hub
unsigned long lastMillis = 0;

void connectToIoTHub(); // <- predefine connectToIoTHub() for setup()
void onMessageCallback(const MQTT::Publish& msg);

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
    Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());

    //do connect upon WiFi connected
    connectToIoTHub();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
    Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
    Serial.printf("Reason: %d\n", event_info.reason);
}

void onClientEvent(const AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {
    if (event == AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEventConnected) {

        Serial.println("Connected to Azure IoT Hub");

        //Add the callback to process cloud-to-device message/command
        client.onMessage(onMessageCallback);
    }
}

void onActivateRelayCommand(String cmdName, JsonVariant jsonValue) {

    //Parse cloud-to-device message JSON. In this example, I send the command message with following format:
    //{"Name":"ActivateRelay","Parameters":{"Activated":0}}

    JsonObject& jsonObject = jsonValue.as<JsonObject>();
    if (jsonObject.containsKey("Parameters")) {
        auto params = jsonValue["Parameters"];
        auto isAct = (params["Activated"]);
        if (isAct) {
            Serial.println("Activated true");
            digitalWrite(LED_PIN, HIGH); //visualize relay activation with the LED
        }
        else {
            Serial.println("Activated false");
            digitalWrite(LED_PIN, LOW);
        }
    }
}

void setup() {

    Serial.begin(115200);

    while(!Serial) {
        yield();
    }
    delay(2000);

    Serial.setDebugOutput(true);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

#if USE_BMP180
    if (bmp.begin()) {
        Serial.println("BMP INIT SUCCESS");
    }
#endif

    Serial.print("Connecting to WiFi...");
    //Begin WiFi joining with provided Access Point name and password
    WiFi.begin(AP_SSID, AP_PASS);

    //Handle WiFi events
    e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start the Client
    e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

    //Handle client events
    client.onEvent(onClientEvent);

    //Add command to handle and its handler
    //Command format is assumed like this: {"Name":"[COMMAND_NAME]","Parameters":[PARAMETERS_JSON_ARRAY]}
    client.onCloudCommand("ActivateRelay", onActivateRelayCommand);
}

void onMessageCallback(const MQTT::Publish& msg) {

    //Handle Cloud to Device message by yourself.

//    if (msg.payload_len() == 0) {
//        return;
//    }

//    Serial.println(msg.payload_string());
}

void connectToIoTHub() {

    Serial.print("\nBeginning Azure IoT Hub Client... ");
    if (client.begin()) {
        Serial.println("OK");
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

    //MUST CALL THIS in loop()
    client.run();

    if (client.connected()) {

        // Publish a message roughly every 3 second. Only after time is retrieved and set properly.
        if(millis() - lastMillis > 3000 && timeStatus() != timeNotSet) {
            lastMillis = millis();

            //Read the actual temperature from sensor
            float temp, press;
            readSensor(&temp, &press);

            //Get current timestamp, using Time lib
            time_t currentTime = now();

            // You can do this to publish payload to IoT Hub
            /*
            String payload = "{\"DeviceId\":\"" + String(DEVICE_ID) + "\", \"MTemperature\":" + String(temp) + ", \"EventTime\":" + String(currentTime) + "}";
            Serial.println(payload);

            //client.publish(MQTT::Publish("devices/" + String(DEVICE_ID) + "/messages/events/", payload).set_qos(1));
            client.sendEvent(payload);
            */

            //Or instead, use this more convenient way
            AzureIoTHubMQTTClient::KeyValueMap keyVal = {{"MTemperature", temp}, {"MPressure", press}, {"DeviceId", DEVICE_ID}, {"EventTime", currentTime}};
            client.sendEventWithKeyVal(keyVal);
        }
    }
    else {

    }

    delay(10); // <- fixes some issues with WiFi stability
}
