# AzureIoTHubMQTTClient
It's unofficial Azure IoT Hub client library for **ESP8266**. The main class `AzureIoTHubMQTTClient` extends `PubSubClient` class in order to access Azure IoT Hub using MQTT protocol. 

[PubSubClient](https://github.com/Imroy/pubsubclient) I use is the one that's optimized for ESP8266, and I included all classes into this project to ease the pain, hope it's OK. 
I also use [NTPClientLib](https://github.com/gmag11/NtpClient) to get current timestamp from NTP server, and use the timestamp to determine the SAS token expiration time. All needed classes are also included in this repo.

I try to write the client library to be as familiar as possible. I name the method `sendEvent` instead of `publish`, so it's similar as another Azure IoT Hub client library. If you're familiar with MQTT, you know you should use `topic` in order to publish payload to MQTT broker. But since the MQTT topic used to publish message to Azure IoT Hub cannot be arbitrary, library sets it for you.


## Why?
You know how hard/cryptic Azure IoT Hub client library for Arduino (and ESP8266) is, right? Well, at least to me. It's written in C (remember [the song](https://www.youtube.com/watch?v=wJ81MZUlrDo)?).

Hey, Azure IoT Hub already supports MQTT protocol since the GA, doesn't it? Why don't we just access it using MQTT? Yes, you can, go ahead and deal with that SAS token :)

I took a liberty to write an easy to use class `AzureIoTHubMQTTClient`, all you need to provide are:

* Azure IoT Hub name --> the name you use to create the IoT Hub
* Device ID --> the ID you use to register a device, either via `DeviceExplorer` or `iothub-explorer`
* Device Key --> the primary key that you can get using DeviceExplorer app or using `iothub-explorer get "[device_id]" --connection-string`


## Get started
First thing first, you should have already set up an Azure IoT Hub and add a device. To create your Microsoft Azure IoT Hub and add a device, follow the instructions outlined in the [Setup IoT Hub Microsoft Azure Iot SDK page](https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md).
Then, download this library and add to your Arduino IDE like usual. Along with that, you also need to install following dependency libraries, either manually or via Library Manager:

* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [Time](https://github.com/PaulStoffregen/Time)

To get started, just take a look the example `TempToIoTHub.ino` sketch. Change the followings:

* `[YOUR_SSID_NAME]` and `[YOUR_SSID_PASS]` to connect to WiFi
* `[YOUR_IOTHUB_NAME]`
* `[YOUR_DEVICE_ID]`
* `[YOUR_DEVICE_KEY]`

Upload the sketch to your ESP8266 board. I'll recommend to use the ESP8266-based dev board designed by my team: [ESPectro](http://makestro.com/espectro), it will make your maker life easier :P

And after that, (hopefully) you're good to go. Hey, would be great if you can connect a temperature sensor as the sketch is about reading temperature data and publish it to Azure IoT Hub. I use BMP180 and [Adafruit's BMP085/BMP180 library](https://github.com/adafruit/Adafruit-BMP085-Library). You can change `#define USE_BMP180` from `1` to `0` if you don't have the sensor and let the random on your side.

Or... get a PlatformIO-based sample project from [here](https://github.com/andriyadi/AzureIoTHubMQTTClientSample)

Enjoy!


## Credits

* [PubSubClient](https://github.com/Imroy/pubsubclient)
* [NTPClient](https://github.com/gmag11/NtpClient)
* Creating Azure SAS Token. Adapted from [this code](https://github.com/gloveboxes/Arduino-ESP8266-Secure-Azure-IoT-Hub-Client/blob/master/AzureClient/Publish.ino)
