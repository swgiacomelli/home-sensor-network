#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define WIFI_CONNECTION_TIMEOUT 180000
#define WIFI_CONNECTION_DELAY 500
#define WIFI_CLIENT_CLASS WiFiClient

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

    print("\r\nConnected - IP address: ");
    print(WiFi.localIP().toString());
    print("\r\n");
  }
};

#define WIFI_CONNECT \
  wifi_manager_t::Connect(&Settings, [](auto s) { Serial.print(s); });
