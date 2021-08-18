#define USE_BME280

// TODO: The SECURE_MQTT does not fully implement verification and should be
// considered non-production ready

// #define SECURE_MQTT  // comment out to use a non secured mqtt server

#include "wifi.h"

#include "config.h"
#include "device.h"
#include "mqtt.h"
#include "settings.h"

#define DEVICE_SLEEP_SECONDS 45
#define DEVICE_SLEEP (DEVICE_SLEEP_SECONDS * 1e6)
#define DEVICE_SLEEP_DELAY 1000
#define SENSOR_WAKE_UP_DELAY 1000
#define SENSOR_THROTTLE 30000

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
