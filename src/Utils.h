//
// Created by Andri Yadi on 10/29/16.
//

#ifndef PIOMQTTAZUREIOTHUB_UTILS_H
#define PIOMQTTAZUREIOTHUB_UTILS_H

#include <Arduino.h>

//http://hardwarefun.com/tutorials/url-encoding-in-arduino
String urlEncode(const char* msg);

// http://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
String splitStringByIndex(String data, char separator, int index);

const char *GetValue(const char* value);
const char *GetStringValue(String value);


#endif //PIOMQTTAZUREIOTHUB_UTILS_H
