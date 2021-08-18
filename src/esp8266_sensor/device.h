#pragma once

#include <Arduino.h>
#include <functional>

#if defined(USE_BME280)
#include <Adafruit_BME280.h>
#endif

template <typename D>
struct device_t {
  using DeviceFunction = std::function<bool(D& device)>;
  device_t(DeviceFunction begin, DeviceFunction end)
      : _begin(begin), _end(end) {}

  bool start() { return _begin(_device); }
  bool end() { return _end(_device); }
  D& getDevice() { return _device; }

 private:
  D _device;
  DeviceFunction _begin;
  DeviceFunction _end;
};

#if defined(USE_BME280)
#define BME280_DEVICE_SETUP                                                 \
  device_t<Adafruit_BME280> BME280([](auto& d) { return d.begin(0x76); },   \
                                   [](auto& d) { return true; });           \
  std::unique_ptr<mqtt_t<settings_t>> MQTT(new mqtt_t<settings_t>{          \
      &Settings,                                                            \
      {                                                                     \
          {"temperature",                                                   \
           []() { return BME280.getDevice().readTemperature(); }},          \
          {"pressure", []() { return BME280.getDevice().readPressure(); }}, \
          {"humidity", []() { return BME280.getDevice().readHumidity(); }}, \
      }});

#define BME280_DEVICE_START               \
  if (!BME280.start()) {                  \
    Serial.println("BME280 not working"); \
    ESP.deepSleep(3600e6);                \
  }
#else
#define BME280_DEVICE_SETUP
#define BME280_DEVICE_START
#endif

#define DEVICE_SETUP BME280_DEVICE_SETUP

#define DEVICE_START BME280_DEVICE_START
