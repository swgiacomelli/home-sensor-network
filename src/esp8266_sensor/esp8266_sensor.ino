#define USE_BME280

#include "wifi.h"

#include "config.h"
#include "device.h"
#include "mqtt.h"
#include "settings.h"

#define DEVICE_SLEEP_DELAY 1000
#define SENSOR_WAKE_UP_DELAY 1000
#define SENSOR_THROTTLE 15000

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

  ESP.deepSleep(Settings.deviceSleep());
  // code after this line will not execute if deepSleep is working correctly
  delay(SENSOR_THROTTLE);
}
