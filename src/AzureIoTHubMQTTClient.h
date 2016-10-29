//
// Created by Andri Yadi on 10/29/16.
//

#ifndef PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H
#define PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H

#include "PubSubClient.h"

class AzureIoTHubMQTTClient : public PubSubClient {
public:
    AzureIoTHubMQTTClient(Client& c, String iotHubHostName, String deviceId, String deviceKey);
    ~AzureIoTHubMQTTClient();

    bool connect();
private:
    String iotHubHostName_;
    String deviceId_;
    String deviceKey_;
    String mqttCommandSubscribeTopic_;

    String createIotHubSas(char *key, String url);
};


#endif //PIOMQTTAZUREIOTHUB_AZUREIOTHUBMQTT_H
