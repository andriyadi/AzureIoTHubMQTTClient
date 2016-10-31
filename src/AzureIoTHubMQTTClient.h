//
// Created by Andri Yadi on 10/29/16.
//

#ifndef PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H
#define PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H

#include <Arduino.h>
#include "PubSubClient.h"
#include <ArduinoJson.h>
#undef min
#undef max
#include <functional>
#include <vector>
#include <map>

#define MAX_JSON_OBJECT_SIZE 10

class AzureIoTHubMQTTClient : public PubSubClient {
public:
    AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey);
    ~AzureIoTHubMQTTClient();

    typedef std::map<const char*, JsonVariant> JsonKeyValueMap;

    bool connect();
    bool sendEvent(String payload);
    bool sendEvent(const uint8_t *payload, uint32_t plength, bool retained = false);
    void sendEventWithKeyVal(JsonKeyValueMap keyValMap);

    AzureIoTHubMQTTClient& onMessage(callback_t cb) { onSubscribeCallback_ = cb; return *this; }

private:
    String iotHubHostName_;
    String deviceId_;
    String deviceKey_;
    String mqttCommandSubscribeTopic_, mqttCommandPublishTopic_;
    PubSubClient::callback_t onSubscribeCallback_;

    String createIotHubSas(char *key, String url);
    void _onActualMqttMessageCallback(const MQTT::Publish& p);
};


#endif //PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H
