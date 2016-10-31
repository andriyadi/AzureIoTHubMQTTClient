//
// Created by Andri Yadi on 10/29/16.
//

#include "AzureIoTHubMQTTClient.h"

#include "sha256.h"
#include "Base64.h"
#include "Utils.h"

AzureIoTHubMQTTClient::AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey):
        iotHubHostName_(iotHubHostName), deviceId_(deviceId), deviceKey_(deviceKey), PubSubClient(c, iotHubHostName, 8883) {

    mqttCommandSubscribeTopic_ = "devices/" + deviceId + "/messages/devicebound/#";
    mqttCommandPublishTopic_ = "devices/" + deviceId + "/messages/events/";
}

AzureIoTHubMQTTClient::~AzureIoTHubMQTTClient() {

}

String AzureIoTHubMQTTClient::createIotHubSas(char *key, String url){
    String stringToSign = url + "\n" + 1737504000;

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
    return "sr=" + url + "&sig="+ urlEncode(encodedSign) + "&se=" + 1737504000;
    // END: create SAS
}

bool AzureIoTHubMQTTClient::connect() {

    String mqttUname =  iotHubHostName_ + "/" + deviceId_ + "/DeviceClientType=0.1.0";
    String url = iotHubHostName_ + urlEncode(String("/devices/" + deviceId_).c_str());
    url.toLowerCase();

    char *devKey = (char *)deviceKey_.c_str();

    //TODO: Optimize this so that we don't have to recreate SAS Token on every connection attempt
    String sas = createIotHubSas(devKey, url);
    String mqttPassword = "SharedAccessSignature " + sas;
    //Serial.println(mqttPassword);

    MQTT::Connect conn = MQTT::Connect(deviceId_).set_auth(mqttUname, mqttPassword);//.set_clean_session();
    bool ret = PubSubClient::connect(conn);

    if (ret) {

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

void AzureIoTHubMQTTClient::sendEventWithKeyVal(JsonKeyValueMap keyValMap) {
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
    Serial.println(jsonStr);

    sendEvent(jsonStr);
}

void AzureIoTHubMQTTClient::_onActualMqttMessageCallback(const MQTT::Publish &publish) {

    //Do something here?

    //Last resort
    if (onSubscribeCallback_) {
        onSubscribeCallback_(publish);
    }
}
