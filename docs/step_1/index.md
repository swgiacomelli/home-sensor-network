# Configuring Sensors

## Configure the Arduinio IDE

1. Download the Arduino IDE from https://www.arduino.cc/en/software.
2. Open File->Preferences and add http://arduino.esp8266.com/stable/package_esp8266com_index.json to the Additional boards manager URLs.
![Arduino Preferences](/docs/images/Arduino_Preferences.png)
3. Connect the ESP8266 device via USB to your computer and then select the board and associated COM port.
4. In the event that the computer does not detect your board, it is possible that you need to install a missing driver or that you need to simply restart the Arduino IDE.
5. For this tutorial I am using a [Keyestudio ESP8266 WI-FI Development Board](https://www.keyestudio.com/products/keyestudio-esp8266-wi-fi-development-boardusb-cable-for-arduino-based-on-esp8266-12fwifi-support-rtos). The [Keyestudio Wiki](https://wiki.keyestudio.com/Ks0354_Keyestudio_ESP8266_WI-FI_Development_Board) gives a more detailed explanation of how to get started using this particular development board.

## Connecting a Sensor

In order to be able to publish sensor data, a sensor needs to be connected to the device. For the sensor, I am going to be using a [BMP280](https://www.aliexpress.com/item/32680504702.html) connected via the I2C interface. The BMP280 is a relatively inexpensive sensor that provides temperature, humidity and pressure readings. *Note: although the sensor may actually be a BMP280 and not a BME280, the sensor library that we will be using only works if it is told it is a BME280 -- so a little bit of double think is required.* Despite what the documentation says for the Keyestudio ESP8266 WI-FI Development Board, the I2C sensor needs to be connected as shown below:

![BMP280 Connection Diagram](/docs/images/iot_bmp280_connection.png)

## Software Flowchart

The following illustrates the flow of the sensor software:

![Software Flowchart](/docs/images/iot_software_flow.png)

## Required Libraries

The following libraries are used by this sketch:

* [Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library)
* [PubSubClient](https://github.com/knolleary/pubsubclient)
* [FirebaseJson](https://github.com/mobizt/FirebaseJson)
* [NoDelay](https://github.com/M-tech-Creations/NoDelay)

## Main Sketch

The main sensor software sketch is as follows:

```cpp
#define USE_BME280

#include <ESP8266WiFi.h>

#include "config.h"
#include "device.h"
#include "mqtt.h"
#include "settings.h"
#include "wifi.h"

#define DEVICE_SLEEP_SECONDS 45
#define DEVICE_SLEEP (DEVICE_SLEEP_SECONDS * 1e6)
#define DEVICE_SLEEP_DELAY 1000
#define SENSOR_WAKE_UP_DELAY 1000
#define SENSOR_THROTTLE 30000

WIFI_CLIENT_CLASS wifiClient;
SETTING_DECL

DEVICE_SETUP

void setupDevice() {
  DEVICE_START
  WIFI_CONNECT
  START_MQTT
}

void setup() {
  CONFIGURE_DEVICE
}

void loop() {
  delay(SENSOR_WAKE_UP_DELAY);
  MQTT->loop();
  delay(DEVICE_SLEEP_DELAY);

  ESP.deepSleep(DEVICE_SLEEP);
  // code after this line will not execute if deepSleep is working correctly
  delay(SENSOR_THROTTLE);
}
```

Through the use of *magic* macros most of the detail of the software is obfuscated allowing most of the details of the implementation to be simplified to avoid tight coupling between components and underlying libraries.

The first part of the code includes the various components:

* Configuration Webserver ([config.h](../../src/esp8266_sensor/config.h))
* Device Manager ([device.h](../../src/esp8266_sensor/device.h))
* MQTT Manager ([mqtt.h](../../src/esp8266_sensor/mqtt.h))
* Settings Manager ([settings.h](../../src/esp8266_sensor/settings.h))
* Network/WiFi Manager ([wifi.h](../../src/esp8266_sensor/wifi.h))
  
It also defines how long the device will go into deep sleep between readings ``DEVICE_SLEEP_SECONDS`` .

Finally the first definition ``#define USE_BME280`` specifies that the software will configure itself to use the BME280 sensor. *Again BME280 and BMP280 are used interchangeably.*

```cpp
#define USE_BME280

#include <ESP8266WiFi.h>

#include "config.h"
#include "device.h"
#include "mqtt.h"
#include "settings.h"
#include "wifi.h"

#define DEVICE_SLEEP_SECONDS 45
#define DEVICE_SLEEP (DEVICE_SLEEP_SECONDS * 1e6)
#define DEVICE_SLEEP_DELAY 1000
#define SENSOR_WAKE_UP_DELAY 1000
#define SENSOR_THROTTLE 30000
```

The second part of the sketch sets up the device as well as declaring global variables, Particularly, ``DEVICE_SETUP`` configures a BME280 device and creates the MQTT topics for temperature, humidity, and pressure. The macro ``CONFIGURE DEVICE`` decides if the device has been configured, if not it runs the configuration webserver otherwise ``void setupDevice()1`` is called to configure the device to publish sensor readings.

```cpp
WIFI_CLIENT_CLASS wifiClient;
SETTING_DECL

DEVICE_SETUP

void setupDevice() {
  DEVICE_START
  WIFI_CONNECT
  START_MQTT
}

void setup() {
  CONFIGURE_DEVICE
}
```

Lastly, the sketch runs the loop and publishes the sensor data to the MQTT server; although, this is called in the loop, once ``ESP.deepSleep()`` is called the device will completely reset such that the loop is only run once per publishing cycle.

```cpp
void loop() {
  delay(SENSOR_WAKE_UP_DELAY);
  MQTT->loop();
  delay(DEVICE_SLEEP_DELAY);

  ESP.deepSleep(DEVICE_SLEEP);
  // code after this line will not execute if deepSleep is working correctly
  delay(SENSOR_THROTTLE);
}
```

## Configuration Server

If the device is not configured, a WiFi access point will be setup and a configuration webserver will be run to configure the device. The source code for the configuration server is in [config.h](../../src/esp8266_sensor/config.h) and uses the ESP8266WebServerSecure library to setup a simple server. An html form is served with the auto-generated deviceID and a list of available WiFi networks injected by a separate [javascript file](../../src/esp8266_sensor/assets/config.js). This allows for the html and css files to be stored in the PROGMEM with only the smaller javascript file needed to be dynamically updated. *The server certificate and key are stored in [config_ssl.h](../../src/esp8266_sensor/config_ssl.h) and should be regenerated using the [generate.sh](../../scripts/cert-gen/generate.sh) tool. The HTML files are stored under /src/esp8266_sensor/assets.

* [index.html](../../src/esp8266_sensor/assets/index.max.html)
* [config.css](../../src/esp8266_sensor/assets/config.css)
* [config.js](../../src/esp8266_sensor/assets/config.js)
* [update.html](../../src/esp8266_sensor/assets/update.max.html)
* [update.css](../../src/esp8266_sensor/assets/update.css)

## Device Manager

The device manager is defined in [device.h](../../src/esp8266_sensor/device.h) and it exists to abstract away the details of any library being used to interface with connected devices. For this sketch the BMP280 is being used as the connected device and the macros ``DEVICE_SETUP`` and ``DEVICE_START`` are used to abstract the implementation details. The Adafruit BME280 library is being for the BME280 device interface. *Note: the setup macro specifies that the BME280 is using the alternate address 0x76, other devices might use the primary address in which case the code on line 28 should change to:

```cpp
device_t<Adafruit_BME280> BME280([](auto& d) { return d.begin(); },   \
```

## MQTT Manager

The MQTT manager is defined in [mqtt.h](../../src/esp8266_sensor/mqtt.h) and simply abstracts away the library used to implement the MQTT protocol.

## Settings Manager

The settings manager is defined in [settings.h](../../src/esp8266_sensor/settings.h) and manages the validation and persistence of configuration settings. In this sketch the settings are stored in /settings.json using the LittleFS and FirebaseJson to load and save the settings.

## Network/WiFi Manager

The network/WiFi manager is defined in [wifi.h](../../src/esp8266_sensor/wifi.h) and serves to abstract the network connection details.

## [Next Step: Configuring Backend Services](../step_2/index.md)