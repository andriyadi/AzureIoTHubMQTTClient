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

    using namespace std::placeholders;
    NTP.onNTPSyncEvent(std::bind(&AzureIoTHubMQTTClient::onNTPSynced, this, _1));
}

AzureIoTHubMQTTClient::~AzureIoTHubMQTTClient() {

}

void AzureIoTHubMQTTClient::onNTPSynced(NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
        DEBUGLOG("Time Sync error: ");
        if (ntpEvent == noResponse) {
            DEBUGLOG("NTP server not reachable\n");
        }
        else if (ntpEvent == invalidAddress) {
            DEBUGLOG("Invalid NTP server address\n");
        }

        NTP.setInterval(5); //try again soon

//        ntpTrialCount_++;
//        if (ntpTrialCount_ > 3) {
//        }

    }
    else {
        ntpSyncedFlag_ = true;
        DEBUGLOG("Got NTP time: ");
        DEBUGLOG("%s\n", NTP.getTimeDateString(NTP.getLastNTPSync()).c_str());
        NTP.setInterval(61);

        changeEventTo(AzureIoTHubMQTTClientEventNTPSynced);

    }

    DEBUGLOG("Current timestamp: %d\n", now());
}

bool AzureIoTHubMQTTClient::begin() {

    if (!WiFi.isConnected())	{
        DEBUGLOG("NOT connected to internet!\n");
        return false;
    }

    NTP.begin(NTP_DEFAULT_HOST);//, 1, true);
    changeEventTo(AzureIoTHubMQTTClientEventNTPSyncing);

    return true;
}

void AzureIoTHubMQTTClient::run() {

    //if (ntpSyncedFlag_ || timeStatus() == timeSet) {
    if (ntpSyncedFlag_ && currentEvent_ == AzureIoTHubMQTTClientEventNTPSynced) {
        ntpSyncedFlag_ = false;

        if (!connected()) {
            doConnect();
        }
    }
    else {
        if (currentEvent_ == AzureIoTHubMQTTClientEventNTPSyncing) {
            timeStatus();
        }
    }

    PubSubClient::loop();
}

void AzureIoTHubMQTTClient::end() {
    //ntpTrialCount_ = 0;
    disconnect();
    NTP.stop();
}

String AzureIoTHubMQTTClient::createIotHubSASToken(char *key, String url, long expire){

    url.toLowerCase();
    if (expire == 0) {
        expire = 1737504000; //hardcoded expire
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

    if (sasToken_.equals("")) {
        DEBUGLOG("Creating SAS Token!\n");

        String url = iotHubHostName_ + urlEncode(String("/devices/" + deviceId_).c_str());
        char *devKey = (char *)deviceKey_.c_str();
        long expire = (timeStatus() == timeSet? now(): 0) + (AZURE_IOTHUB_TOKEN_EXPIRE);
        DEBUGLOG("SAS Token expire: %d\n", expire);

        //TODO: Store SAS token? So that no expensive operation for each begin
        sasToken_ = createIotHubSASToken(devKey, url, expire);
    }

    changeEventTo(AzureIoTHubMQTTClientEventConnecting);

    String mqttUname =  iotHubHostName_ + "/" + deviceId_ + "/api-version=2016-11-14";
    String mqttPassword = "SharedAccessSignature " + sasToken_;
    //DEBUGLOG(mqttPassword);

    MQTT::Connect conn = MQTT::Connect(deviceId_).set_auth(mqttUname, mqttPassword);//.set_clean_session();
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

bool AzureIoTHubMQTTClient::setTimeZone(int timeZone) {
    return NTP.setTimeZone(timeZone);
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
