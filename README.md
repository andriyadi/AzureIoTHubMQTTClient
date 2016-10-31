# AzureIoTHubMQTTClient
It's unofficial Azure IoT Hub client library for ESP8266. The main class `AzureIoTHubMQTTClient` extends `PubSubClient` class in order to access Azure IoT Hub using MQTT protocol. 

[PubSubClient](https://github.com/Imroy/pubsubclient) I use is the one that's optimized for ESP8266, and I included all classes in this project to ease the pain, hope it's OK. 

## Why?
You know how hard/cryptic Azure IoT Hub client for Arduino (and ESP8266) is, right? Well, at least to me. It's written in C (remember [the song](https://www.youtube.com/watch?v=wJ81MZUlrDo)?).

Hey, Azure IoT Hub already supports MQTT protocol since the GA, doesn't it? Why don't we just access it using MQTT? Yes, you can, go ahead and deal with that SAS token :)

I took a liberty to write an easy to use class `AzureIoTHubMQTTClient`, all you need to provide are:

* Azure IoT Hub name
* Device ID
* Device Key --> the primary key that you can get using DeviceExplorer app or using `iothub-explorer get "[device_id]" --connection-string`


## Get started
First thing first, you should have already set up an Azure IoT Hub and add a device. To create your Microsoft Azure IoT Hub and add a device, follow the instructions outlined in the [Setup IoT Hub Microsoft Azure Iot SDK page](https://github.com/Azure/azure-iot-sdks/blob/master/doc/setup_iothub.md).
Then, download this library and add to your Arduino IDE like usual. To get started, just take a look the example `TempToIoTHub.ino` sketch. Change following:

* `[YOUR_SSID_NAME]` and `[YOUR_SSID_PASS]` to connect to WiFi
* `[YOUR_IOTHUB_NAME]`
* `[YOUR_DEVICE_ID]`
* `[YOUR_DEVICE_KEY]`

In the example sketch I use [NTPClientLib](https://github.com/gmag11/NtpClient) in order to get the real timestamp. Follow NTPClientLib instruction to use it.

Upload the sketch to your ESP8266 board. Of course I'll recommend to use my company's ESP8266-based dev board: [ESPectro](http://makestro.com/espectro), it will make your maker life easier :P

And after that, (hopefully) you're good to go. Hey, would be great if you can connect a temperature sensor as the sketch is about reading temperature data and publish it to Azure IoT Hub. I use BMP180 and [Adafruit's BMP085/BMP180 library](https://github.com/adafruit/Adafruit-BMP085-Library). You can change `#define USE_BMP180` to `0` if you don't have the sensor and let the random on your side.

Enjoy!
