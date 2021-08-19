#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <TZ.h>
#include <time.h>

#include <vector>

#define WIFI_CONNECTION_TIMEOUT 180000
#define WIFI_CONNECTION_DELAY 500

struct wifi_manager_t {
  template <typename S>
  static void Connect(S* settings, auto&& print) {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(settings->deviceID + ".iot");
    WiFi.begin(settings->wifiSSID, settings->wifiPassword);

    auto wifi_timeout = WIFI_CONNECTION_TIMEOUT / WIFI_CONNECTION_DELAY;

    print("Connecting to ");
    print(settings->wifiSSID);

    while (WiFi.status() != WL_CONNECTED) {
      delay(WIFI_CONNECTION_DELAY);
      print(".");
      if (--wifi_timeout <= 0) {
        print("\r\n");
        print("Error connecting to ");
        print(settings->wifiSSID);
        print("\r\n");
        print("Device going to configuration mode.\r\n");
        settings->reset();
        ESP.reset();
      }
    }

    print("\r\n");
    SetupTime(print);

    print("Connected - IP address: ");
    print(WiFi.localIP().toString());
    print("\r\n");
  }

  static std::vector<String> ScanNetworks() {
    auto networks = std::vector<String>{};

    auto scanResult = WiFi.scanNetworks(false, false);
    for (auto i = 0; i < scanResult; i++) {
      String ssid;
      int32_t rssi;
      uint8_t encryptionType;
      uint8_t* bssid;
      int32_t channel;
      bool hidden;
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel,
                          hidden);
      networks.push_back(ssid);
    }
    return networks;
  }

  static void SetupTime(auto&& print) {
    print("Setting time ");
    configTime(TZ_Etc_UTC, "pool.ntp.org");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
      delay(500);
      print(".");
      now = time(nullptr);
    }
    print("\r\n");
    print("Time set\r\n");
  }
};

#define WIFI_CONNECT \
  wifi_manager_t::Connect(&Settings, [](auto s) { Serial.print(s); });
