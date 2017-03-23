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

#define DEBUG_PORT Serial
#define DEBUG

#ifdef DEBUG
#define DEBUG_NTPCLIENT
#define DEBUGLOG(...) DEBUG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

#include "NtpClientLib.h"

#define MAX_JSON_OBJECT_SIZE 20
#define NTP_DEFAULT_HOST     DEFAULT_NTP_SERVER //"time.nist.gov"//"pool.ntp.org"
#define AZURE_IOTHUB_MQTT_PORT    8883
#define AZURE_IOTHUB_TOKEN_EXPIRE   10*24*3600 //seconds

class AzureIoTHubMQTTClient : public PubSubClient {
public:

    enum AzureIoTHubMQTTClientEvent {
        AzureIoTHubMQTTClientEventUnknown,
        AzureIoTHubMQTTClientEventNTPSyncing,
        AzureIoTHubMQTTClientEventNTPSynced,
        AzureIoTHubMQTTClientEventConnecting,
        AzureIoTHubMQTTClientEventConnected,
        AzureIoTHubMQTTClientEventDisconnected
    };

    typedef std::function<void(const AzureIoTHubMQTTClientEvent event)> EventCallback;

    typedef std::function<void(String, JsonVariant)> AzureIoTHubMQTTClientCommandCallback;
    typedef std::map<String, AzureIoTHubMQTTClientCommandCallback> CommandsHandlerMap_t;

    AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey);
    ~AzureIoTHubMQTTClient();

    typedef std::map<const char*, JsonVariant> KeyValueMap;

    bool begin();
    void run();
    void end();
    bool sendEvent(String payload);
    bool sendEvent(const uint8_t *payload, uint32_t plength, bool retained = false);
    void sendEventWithKeyVal(KeyValueMap keyValMap);
    bool setTimeZone(int timeZone);

    void onEvent(EventCallback cb) {
        eventCallback_ = cb;
    }

    void onCloudCommand(String command, AzureIoTHubMQTTClientCommandCallback callback);

    AzureIoTHubMQTTClient& onMessage(callback_t cb) { onSubscribeCallback_ = cb; return *this; }

private:
    String iotHubHostName_;
    String deviceId_;
    String deviceKey_;
    String sasToken_;
    String mqttCommandSubscribeTopic_, mqttCommandPublishTopic_;
    PubSubClient::callback_t onSubscribeCallback_;
    volatile bool ntpSyncedFlag_ = false;

    bool parseMessageAsJson_ = false;

    EventCallback eventCallback_ = NULL;
    CommandsHandlerMap_t commandsHandlerMap_;

    bool doConnect();
    String createIotHubSASToken(char *key, String url, long expire = 0);
    void _onActualMqttMessageCallback(const MQTT::Publish& p);
    void onNTPSynced(NTPSyncEvent_t ntpEvent);
    //uint8_t ntpTrialCount_ = 0;

    AzureIoTHubMQTTClientEvent currentEvent_ = AzureIoTHubMQTTClientEventUnknown;
    void changeEventTo(AzureIoTHubMQTTClientEvent event);
};


#endif //PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H
