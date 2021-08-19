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

#define DEFAULT_DEVICE_SLEEP_SECONDS 45

#define FILESYSTEM LittleFS
#define DEVICE_ID_PREFIX "IOT_"

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

namespace settings {
const char settings_file[] = "/settings.json";
}

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
  int deviceSleepSeconds = DEFAULT_DEVICE_SLEEP_SECONDS;

  long deviceSleep() { return deviceSleepSeconds * 1e6; }

  void from_json(const String& data) {
    FirebaseJson json;
    auto get_value = [&json](const String& key) {
      FirebaseJsonData result;
      json.get(result, key);
      return result;
    };

    json.setJsonData(data);

    deviceID = get_value("deviceID").stringValue;

    wifiSSID = get_value("wifiSSID").stringValue;
    wifiPassword = get_value("wifiPassword").stringValue;

    mqttServer = get_value("mqttServer").stringValue;

    mqttPort = (uint16_t)get_value("mqttPort").intValue;
    if (!mqttPort) {
      mqttPort = (uint16_t)get_value("mqttPort").stringValue.toInt();
    }

    mqttUsername = get_value("mqttUsername").stringValue;
    mqttPassword = get_value("mqttPassword").stringValue;

    deviceSleepSeconds = get_value("deviceSleepSeconds").intValue;

    // lazy merge
    if (deviceSleepSeconds <= 0) {
      deviceSleepSeconds = DEFAULT_DEVICE_SLEEP_SECONDS;
    }

    trim();
  }

  String to_json() {
    FirebaseJson json;

    json.set("deviceID", deviceID);

    json.set("wifiSSID", wifiSSID);
    json.set("wifiPassword", wifiPassword);

    json.set("mqttServer", mqttServer);
    json.set("mqttPort", mqttPort);
    json.set("mqttUsername", mqttUsername);
    json.set("mqttPassword", mqttPassword);

    json.set("deviceSleepSeconds", deviceSleepSeconds);

    String output;
    json.toString(output);
    return output;
  }

  void set_default_values() {
    ensureDeviceID();

    wifiSSID = "";
    wifiPassword = "";

    mqttServer = "";
    mqttPort = MQTT_DEFAULT_PORT;
    mqttUsername = "";
    mqttPassword = "";

    deviceSleepSeconds = DEFAULT_DEVICE_SLEEP_SECONDS;
  }

 private:
  bool _fsStarted = false;

 public:
  void trim() {
    deviceID.trim();
    wifiSSID.trim();
    wifiPassword.trim();
    mqttServer.trim();
    mqttUsername.trim();
    mqttPassword.trim();
  }

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

    deviceState = device_state::configured;
  }

  void reset() {
    set_default_values();
    save();
  }

  bool validate() {
    trim();

    return !(deviceID.isEmpty() || wifiSSID.isEmpty() || mqttServer.isEmpty() ||
             mqttPort <= 0 || deviceSleepSeconds <= 0);
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

settings_t Settings;