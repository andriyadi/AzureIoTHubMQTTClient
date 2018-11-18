//
// Created by Andri Yadi on 10/29/16.
//
#pragma once

#include <Arduino.h>
#include "PubSubClient.h"
#include <ArduinoJson.h>
#undef min
#undef max
#include <functional>
#include <vector>
#include <map>

#define DEBUG_PORT Serial
#define DEBUG

#ifdef DEBUG
#define DEBUGLOG(...) DEBUG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

#define MAX_JSON_OBJECT_SIZE 20
#define AZURE_IOTHUB_MQTT_PORT    8883

class AzureIoTHubMQTTClient : public PubSubClient {
public:

    enum AzureIoTHubMQTTClientEvent {
        AzureIoTHubMQTTClientEventUnknown,
        AzureIoTHubMQTTClientEventConnecting,
        AzureIoTHubMQTTClientEventConnected,
        AzureIoTHubMQTTClientEventDisconnected
    };

    typedef std::function<void(const AzureIoTHubMQTTClientEvent event)> EventCallback;
    typedef std::function<void(String, JsonVariant)> AzureIoTHubMQTTClientCommandCallback;
    typedef std::map<String, AzureIoTHubMQTTClientCommandCallback> CommandsHandlerMap_t;
    typedef std::map<const char*, JsonVariant> KeyValueMap;

    AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey);
    ~AzureIoTHubMQTTClient() { }

    void run();
    void end();
    bool sendEvent(String payload);
    void sendEventWithKeyVal(KeyValueMap keyValMap);

    void onEvent(EventCallback cb) {
        eventCallback_ = cb;
    }

    void onCloudCommand(String command, AzureIoTHubMQTTClientCommandCallback callback);

    AzureIoTHubMQTTClient& onMessage(callback_t cb) { onSubscribeCallback_ = cb; return *this; }

private:
    String iotHubHostName_;
    String deviceId_;
    String deviceKey_;
    String mqttCommandPublishTopic_;
    PubSubClient::callback_t onSubscribeCallback_;

    bool parseMessageAsJson_ = false;

    EventCallback eventCallback_ = NULL;
    CommandsHandlerMap_t commandsHandlerMap_;

    bool doConnect();
    void _onActualMqttMessageCallback(const MQTT::Publish& p);

    AzureIoTHubMQTTClientEvent currentEvent_ = AzureIoTHubMQTTClientEventUnknown;
    void changeEventTo(AzureIoTHubMQTTClientEvent event);
};