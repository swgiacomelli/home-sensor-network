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
// settings.h abstracts away configuration settings management.

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>

#include "json.h"

#if defined(SECURE_MQTT)
#define MQTT_DEFAULT_PORT 8883
#else
#define MQTT_DEFAULT_PORT 1883
#endif

#define DEFAULT_DEVICE_SLEEP_SECONDS 45

#define FILESYSTEM LittleFS
#define DEVICE_ID_PREFIX "IOT_"

#define CONFIGURE_DEVICE                                                      \
  Serial.begin(115200);                                                       \
  Serial.println();                                                           \
  Settings.init();                                                            \
  if (Settings.deviceState() == device_state::unconfigured) {                 \
    configuration_server_t<settings_t>::Run(&Settings,                        \
                                            [](auto v) { Serial.print(v); }); \
  }                                                                           \
  setupDevice();

namespace {
String generateDeviceID() {
  uint8_t mac[6];
  char macStr[13] = {0};
  wifi_get_macaddr(STATION_IF, mac);

  snprintf(macStr, 13, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);

  return String(DEVICE_ID_PREFIX) + String(macStr);
}
}  // namespace

namespace settings {
const char settings_file[] = "/settings.json";
}

enum class device_state { unknown, unconfigured, configured };

struct settings_t {
  settings_t()
      : _deviceState(device_state::unknown),
        _deviceID(),
        _wifiSSID(),
        _wifiPassword(),
        _mqttServer(),
        _mqttUsername(),
        _mqttPassword(),
        _mqttPort(MQTT_DEFAULT_PORT),
        _deviceSleepSeconds(DEFAULT_DEVICE_SLEEP_SECONDS) {}

  String& deviceID() { return _deviceID; }
  void deviceID(const String& value) {
    _deviceID = value;
    _deviceID.trim();
  }

  device_state deviceState() { return _deviceState; }
  void deviceState(const device_state value) { _deviceState = value; }

  String& wifiSSID() { return _wifiSSID; }
  void wifiSSID(const String& value) {
    _wifiSSID = value;
    _wifiSSID.trim();
  }
  String& wifiPassword() { return _wifiPassword; }
  void wifiPassword(const String& value) {
    _wifiPassword = value;
    _wifiPassword.trim();
  }

  String& mqttServer() { return _mqttServer; }
  void mqttServer(const String& value) {
    _mqttServer = value;
    _mqttServer.trim();
  }
  String& mqttUsername() { return _mqttUsername; }
  void mqttUsername(const String& value) {
    _mqttUsername = value;
    _mqttUsername.trim();
  }
  String& mqttPassword() { return _mqttPassword; }
  void mqttPassword(const String& value) {
    _mqttPassword = value;
    _mqttPassword.trim();
  }
  uint16_t mqttPort() { return _mqttPort; }
  void mqttPort(const uint16_t value) { _mqttPort = value; }

  long deviceSleep() { return _deviceSleepSeconds * 1e6; }
  int deviceSleepSeconds() { return _deviceSleepSeconds; }
  void deviceSleepSeconds(const int value) { _deviceSleepSeconds = value; }

  void from_json(const String& data) {
    json_t json;
    if (!json.loads(data)) {
      return;
    }

    json.get_if_exists<String>("deviceID", [&](auto v) { deviceID(v); });

    json.get_if_exists<String>("wifiSSID", [&](auto v) { wifiSSID(v); });
    json.get_if_exists<String>("wifiPassword",
                               [&](auto v) { wifiPassword(v); });

    json.get_if_exists<String>("mqttServer", [&](auto v) { mqttServer(v); });
    json.get_if_exists<int>("mqttPort", [&](auto v) { mqttPort(v); });
    json.get_if_exists<String>("mqttUsername",
                               [&](auto v) { mqttUsername(v); });
    json.get_if_exists<String>("mqttPassword",
                               [&](auto v) { mqttPassword(v); });

    json.get_if_exists<int>("deviceSleepSeconds",
                            [&](auto v) { deviceSleepSeconds(v); });
  }

  String to_json() {
    json_t json;

    json.set("deviceID", deviceID());

    json.set("wifiSSID", wifiSSID());
    json.set("wifiPassword", wifiPassword());

    json.set("mqttServer", mqttServer());
    json.set("mqttPort", mqttPort());
    json.set("mqttUsername", mqttUsername());
    json.set("mqttPassword", mqttPassword());

    json.set("deviceSleepSeconds", deviceSleepSeconds());

    return json.dumps();
  }

  void set_default_values() {
    ensureDeviceID();

    wifiSSID("");
    wifiPassword("");

    mqttServer("");
    mqttPort(MQTT_DEFAULT_PORT);
    mqttUsername("");
    mqttPassword("");

    deviceSleepSeconds(DEFAULT_DEVICE_SLEEP_SECONDS);
  }

 private:
  bool _fsStarted = false;

 public:
  void init() {
    ensureDeviceID();

    if (!_fsStarted) {
      if (!FILESYSTEM.begin()) {
        setUnconfigured();
        return;
      }
      _fsStarted = true;
    }

    auto f = FILESYSTEM.open(settings::settings_file, "r");
    if (f) {
      from_json(f.readString());
      f.close();
    } else {
      setUnconfigured();
      return;
    }

    if (validate()) {
      deviceState(device_state::configured);
    } else {
      setUnconfigured();
    }
  }

  void save() {
    if (!_fsStarted) {
      if (!FILESYSTEM.begin()) {
        if (!FILESYSTEM.format()) {
          setUnconfigured();
          return;
        }
        if (!FILESYSTEM.begin()) {
          setUnconfigured();
          return;
        }
      }
      _fsStarted = true;
    }

    auto f = FILESYSTEM.open(settings::settings_file, "w");
    if (!f) {
      setUnconfigured();
      return;
    }

    auto output = to_json();
    f.print(output);
    f.close();

    if (!validate()) {
      setUnconfigured();
      return;
    }

    deviceState(device_state::configured);
  }

  void reset() {
    set_default_values();
    save();
  }

  bool validate() {
    return !(deviceID().isEmpty() || wifiSSID().isEmpty() ||
             mqttServer().isEmpty() || mqttPort() <= 0 ||
             deviceSleepSeconds() <= 0);
  }

  void setUnconfigured() {
    deviceState(device_state::unconfigured);
    ensureDeviceID();
  }

  void ensureDeviceID() {
    if (deviceID().isEmpty()) {
      deviceID(generateDeviceID());
    }
  }

  device_state _deviceState;
  String _deviceID;

  String _wifiSSID;
  String _wifiPassword;

  String _mqttServer;
  String _mqttUsername;
  String _mqttPassword;
  uint16_t _mqttPort;
  int _deviceSleepSeconds;
};

settings_t Settings;