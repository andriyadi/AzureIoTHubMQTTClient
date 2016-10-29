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

    String mqttUname =  iotHubHostName_ + "/" + deviceId_;
    String url = iotHubHostName_ + urlEncode(String("/devices/" + deviceId_).c_str());
    url.toLowerCase();

    char *devKey = (char *)deviceKey_.c_str();

    String sas = createIotHubSas(devKey, url);
    String mqttPassword = "SharedAccessSignature " + sas;
    Serial.println(mqttPassword);

    bool ret = PubSubClient::connect(MQTT::Connect(deviceId_).set_keepalive(5).set_auth(mqttUname, mqttPassword).set_clean_session());

    if (ret) {
        PubSubClient::subscribe(MQTT::Subscribe().add_topic(mqttCommandSubscribeTopic_));
        return true;
    } else {
        return false;
    }
}
