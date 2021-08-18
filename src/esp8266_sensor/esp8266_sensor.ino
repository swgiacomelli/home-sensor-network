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
