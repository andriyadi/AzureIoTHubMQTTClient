//
// Created by Andri Yadi on 10/29/16.
//

#include "AzureIoTHubMQTTClient.h"

#include <ESP8266WiFi.h>

AzureIoTHubMQTTClient::AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey):
        iotHubHostName_(iotHubHostName), deviceId_(deviceId), deviceKey_(deviceKey), PubSubClient(c, iotHubHostName, AZURE_IOTHUB_MQTT_PORT) {

    mqttCommandPublishTopic_ = "devices/" + deviceId + "/messages/events/";
}

void AzureIoTHubMQTTClient::run() {
    if (!connected()) {
        doConnect();
    }

    PubSubClient::loop();
}

void AzureIoTHubMQTTClient::end() {
    disconnect();
}

bool AzureIoTHubMQTTClient::doConnect() {
    changeEventTo(AzureIoTHubMQTTClientEventConnecting);

    auto mqttUname = iotHubHostName_ + "/" + deviceId_ + "/api-version=2016-11-14";

    auto conn = MQTT::Connect(deviceId_)
        .set_auth(mqttUname, deviceKey_)
        .set_keepalive(10);

    if (PubSubClient::connect(conn)) {
        changeEventTo(AzureIoTHubMQTTClientEventConnected);

        auto cb = [=](const MQTT::Publish& p){
            _onActualMqttMessageCallback(p);
        };

        set_callback(cb);

        //Directly subscribe
        PubSubClient::subscribe("devices/" + deviceId_ + "/messages/devicebound/#");
        return true;

    }
    else {
        DEBUGLOG("Failed to connect to Azure IoT Hub\n");
        return false;
    }
}

bool AzureIoTHubMQTTClient::sendEvent(String payload) {
    return PubSubClient::publish(MQTT::Publish(mqttCommandPublishTopic_, payload).set_qos(1).set_retain(false));
}

void AzureIoTHubMQTTClient::sendEventWithKeyVal(KeyValueMap keyValMap) {
    if (keyValMap.size() == 0) {
        return;
    }

    const int BUFFER_SIZE = JSON_OBJECT_SIZE(MAX_JSON_OBJECT_SIZE);
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    auto& root = jsonBuffer.createObject();

    for (const auto &keyVal: keyValMap) {
        root[keyVal.first] = keyVal.second;
    }

    String jsonStr;
    root.printTo(jsonStr);
    DEBUGLOG("JSON: %s\n", jsonStr.c_str());

    sendEvent(jsonStr);
}

void AzureIoTHubMQTTClient::_onActualMqttMessageCallback(const MQTT::Publish &msg) {
    //Process message
    if (msg.payload_len() > 0 && parseMessageAsJson_ && commandsHandlerMap_.size() > 0) {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)msg.payload(), 3);

        if (json.success()) {
            String key = "";

            if (json.containsKey("Name"))
                key = "Name";
            else if (json.containsKey("name"))
                key = "name";

            if (!key.equals("")) {
                String cmdName = String(json[key].as<char*>());

                for (const auto &myPair : commandsHandlerMap_) {
                    if (!cmdName.equals(myPair.first))
                        continue;

                    DEBUGLOG("Found %s command\n", cmdName.c_str());
                    AzureIoTHubMQTTClientCommandCallback cb = myPair.second;
                    cb(myPair.first, json);
                }
            }
        }
    }

    // Last resort
    if (onSubscribeCallback_)
        onSubscribeCallback_(msg);
}

void AzureIoTHubMQTTClient::onCloudCommand(String command,
                                           AzureIoTHubMQTTClient::AzureIoTHubMQTTClientCommandCallback callback) {
    parseMessageAsJson_ = true;
    commandsHandlerMap_[command] = callback;
}

void AzureIoTHubMQTTClient::changeEventTo(AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {
    currentEvent_ = event;

    if (eventCallback_) {
        eventCallback_(event);
    }
}