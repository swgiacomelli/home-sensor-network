#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <FirebaseJson.h>
#include <LittleFS.h>

#if defined(SECURE_MQTT)
#define MQTT_DEFAULT_PORT 8883
#else
#define MQTT_DEFAULT_PORT 1883
#endif

#define FILESYSTEM LittleFS
#define DEVICE_ID_PREFIX "IOT_"
#define SETTINGS_FILE "/settings.json"

#define SETTING_DECL settings_t Settings;

#define CONFIGURE_DEVICE                                                      \
  Serial.begin(115200);                                                       \
  Serial.println();                                                           \
  Settings.init();                                                            \
  if (Settings.deviceState == device_state::unconfigured) {                   \
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

enum class device_state { unknown, unconfigured, configured };

struct settings_t {
  device_state deviceState = device_state::unknown;
  String deviceID;

  String wifiSSID;
  String wifiPassword;

  String mqttServer;
  String mqttUsername;
  String mqttPassword;
  uint16_t mqttPort = MQTT_DEFAULT_PORT;

 private:
  bool _fsStarted = false;

  FirebaseJson _settingsDoc;

  String getString(const String& key) {
    FirebaseJsonData result;
    if (!_settingsDoc.get(result, key)) {
      return {};
    }
    return result.stringValue;
  }

  int getInt(const String& key) {
    FirebaseJsonData result;
    if (!_settingsDoc.get(result, key)) {
      return {};
    }
    return result.intValue;
  }

  template <typename T>
  void setValue(const String& key, const T& value) {
    _settingsDoc.set(key, value);
  }

  void loadValues() {
    deviceID = getString("deviceID");

    wifiSSID = getString("wifiSSID");
    wifiPassword = getString("wifiPassword");

    mqttServer = getString("mqttServer");
    mqttPort = (uint16_t)getInt("mqttPort");
    mqttUsername = getString("mqttUsername");
    mqttPassword = getString("mqttPassword");
    trim();
  }

  void saveValues(auto& f) {
    _settingsDoc.clear();
    setValue("deviceID", deviceID);

    setValue("wifiSSID", wifiSSID);
    setValue("wifiPassword", wifiPassword);

    setValue("mqttServer", mqttServer);
    setValue("mqttPort", mqttPort);
    setValue("mqttUsername", mqttUsername);
    setValue("mqttPassword", mqttPassword);
    String output;
    _settingsDoc.toString(output);
    f.print(output);
  }

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

    auto f = FILESYSTEM.open(SETTINGS_FILE, "r");
    if (f) {
      _settingsDoc.setJsonData(f.readString());
      loadValues();
      f.close();
    } else {
      setUnconfigured();
      return;
    }

    if (validate()) {
      deviceState = device_state::configured;
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

    auto f = FILESYSTEM.open(SETTINGS_FILE, "w");
    if (!f) {
      setUnconfigured();
      return;
    }

    saveValues(f);

    f.close();

    if (!validate()) {
      setUnconfigured();
      return;
    }

    deviceState = device_state::configured;
  }

  void reset() {
    ensureDeviceID();

    wifiSSID = "";
    wifiPassword = "";

    mqttServer = "";
    mqttPort = MQTT_DEFAULT_PORT;
    mqttUsername = "";
    mqttPassword = "";

    save();
  }

  void trim() {
    deviceID.trim();
    wifiSSID.trim();
    wifiPassword.trim();
    mqttServer.trim();
    mqttUsername.trim();
    mqttPassword.trim();
  }

  bool validate() {
    trim();
    return !(deviceID.isEmpty() || wifiSSID.isEmpty() ||
             mqttServer.isEmpty()) &&
           mqttPort > 0;
  }

  void setUnconfigured() {
    deviceState = device_state::unconfigured;
    ensureDeviceID();
  }

  void ensureDeviceID() {
    deviceID.trim();
    if (deviceID.isEmpty()) {
      deviceID = generateDeviceID();
    }
  }
};