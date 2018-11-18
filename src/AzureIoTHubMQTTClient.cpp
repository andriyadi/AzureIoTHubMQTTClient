//
// Created by Andri Yadi on 10/29/16.
//

#include "AzureIoTHubMQTTClient.h"

#include "sha256.h"
#include "Base64.h"
#include "Utils.h"
#include <ESP8266WiFi.h>

AzureIoTHubMQTTClient::AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey):
        iotHubHostName_(iotHubHostName), deviceId_(deviceId), deviceKey_(deviceKey), PubSubClient(c, iotHubHostName, AZURE_IOTHUB_MQTT_PORT) {

    mqttCommandSubscribeTopic_ = "devices/" + deviceId + "/messages/devicebound/#";
    mqttCommandPublishTopic_ = "devices/" + deviceId + "/messages/events/";
}

AzureIoTHubMQTTClient::~AzureIoTHubMQTTClient() {

}

bool AzureIoTHubMQTTClient::begin() {

    if (!WiFi.isConnected())	{
        DEBUGLOG("NOT connected to internet!\n");
        return false;
    }

    return true;
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

String AzureIoTHubMQTTClient::createIotHubSASToken(char *key, String url, long expire){

    url.toLowerCase();
    if (expire == 0) {
        expire = 2147483647; // hardcode expire to MAX unix timestamp
    }

    String stringToSign = url + "\n" + String(expire);

    // START: Create signature
    // https://raw.githubusercontent.com/adamvr/arduino-base64/master/examples/base64/base64.ino

    int keyLength = strlen(key);

    int decodedKeyLength = base64_dec_len(key, keyLength);
    char decodedKey[decodedKeyLength];  //allocate char array big enough for the base64 decoded key

    base64_decode(decodedKey, key, keyLength);  //decode key

    Sha256.initHmac((const uint8_t*)decodedKey, decodedKeyLength);
    Sha256.print(stringToSign);
    char* sign = (char*) Sha256.resultHmac();
    // END: Create signature

    // START: Get base64 of signature
    int encodedSignLen = base64_enc_len(HASH_LENGTH);
    char encodedSign[encodedSignLen];
    base64_encode(encodedSign, sign, HASH_LENGTH);

    // SharedAccessSignature
    return "sr=" + url + "&sig="+ urlEncode(encodedSign) + "&se=" + String(expire);
    // END: create SAS
}

bool AzureIoTHubMQTTClient::doConnect() {
    changeEventTo(AzureIoTHubMQTTClientEventConnecting);

    String mqttUname = iotHubHostName_ + "/" + deviceId_ + "/api-version=2016-11-14";
    //DEBUGLOG(mqttPassword);

    MQTT::Connect conn = MQTT::Connect(deviceId_).set_auth(mqttUname, deviceKey_);//.set_clean_session();
    conn.set_keepalive(10);
    bool ret = PubSubClient::connect(conn);

    if (ret) {

        //DEBUGLOG("Connected to Azure IoT Hub\n");
        changeEventTo(AzureIoTHubMQTTClientEventConnected);

        PubSubClient::callback_t cb = [=](const MQTT::Publish& p){
            _onActualMqttMessageCallback(p);
        };

        set_callback(cb);

        //Directly subscribe
        PubSubClient::subscribe(mqttCommandSubscribeTopic_);
        //MQTT::Subscribe sub = MQTT::Subscribe(mqttCommandSubscribeTopic_, 1);
        //PubSubClient::subscribe(sub);

        return true;
    } else {

        DEBUGLOG("Failed to connect to Azure IoT Hub\n");
        return false;
    }
}

bool AzureIoTHubMQTTClient::sendEvent(String payload) {
    //return PubSubClient::publish(mqttCommandPublishTopic_, payload);
    return PubSubClient::publish(MQTT::Publish(mqttCommandPublishTopic_, payload).set_qos(1).set_retain(false));
}

bool AzureIoTHubMQTTClient::sendEvent(const uint8_t *payload, uint32_t plength, bool retained) {
    return PubSubClient::publish(mqttCommandPublishTopic_, payload, plength, retained);
}

void AzureIoTHubMQTTClient::sendEventWithKeyVal(KeyValueMap keyValMap) {
    if (keyValMap.size() == 0) {
        return;
    }

    const int BUFFER_SIZE = JSON_OBJECT_SIZE(MAX_JSON_OBJECT_SIZE);
    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

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
            if (json.containsKey("Name")) {
                key = "Name";
            }
            else if (json.containsKey("name")) {
                key = "name";
            }

            if (!key.equals("")) {
                String cmdName = String(json[key].as<char*>());

                for (const auto &myPair : commandsHandlerMap_) {
                    if (!cmdName.equals(myPair.first)) {
                        continue;
                    }

                    DEBUGLOG("Found %s command\n", cmdName.c_str());
                    AzureIoTHubMQTTClientCommandCallback cb = myPair.second;
//                    auto val = json[myPair.first];
//                    cb(myPair.first, val);
                    cb(myPair.first, json);
                }
            }
        }
    }

    //Last resort
    if (onSubscribeCallback_) {
        onSubscribeCallback_(msg);
    }
}

void AzureIoTHubMQTTClient::onCloudCommand(String command,
                                           AzureIoTHubMQTTClient::AzureIoTHubMQTTClientCommandCallback callback) {

    parseMessageAsJson_ = true;
    if (commandsHandlerMap_.size() == 0) {
    }

    commandsHandlerMap_[command] = callback;
}

void AzureIoTHubMQTTClient::changeEventTo(AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {

    currentEvent_ = event;

    if (eventCallback_) {
        eventCallback_(event);
    }
}
