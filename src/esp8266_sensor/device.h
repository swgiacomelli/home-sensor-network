// Home Sensor Network
// Copyright (C) 2021 Steven Giacomelli (steve@giacomelli.ca)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// device.h defines generic handlers for connected peripherals.

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

#if defined(USE_ANALOG)
#define ANALOG_DEVICE_SETUP                                        \
  std::unique_ptr<mqtt_t<settings_t>> MQTT(new mqtt_t<settings_t>{ \
      &Settings,                                                   \
      {                                                            \
          {USE_ANALOG, []() { return analogRead(A0); }},           \
      }});

#define ANALOG_DEVICE_START
#else
#define ANALOG_DEVICE_SETUP
#define ANALOG_DEVICE_START
#endif

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
    ESP.deepSleep(0);                     \
  }
#else
#define BME280_DEVICE_SETUP
#define BME280_DEVICE_START
#endif

#define DEVICE_SETUP ANALOG_DEVICE_SETUP BME280_DEVICE_SETUP

#define DEVICE_START ANALOG_DEVICE_START BME280_DEVICE_START
