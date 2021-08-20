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
// wifi.h abstracts the network management

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <TZ.h>
#include <time.h>

#include <vector>

#define WIFI_CONNECTION_TIMEOUT 180000

#define WIFI_CONNECTION_DELAY 500

struct mdns_service_t {
  String service;
  String proto;
  uint16_t port;
};

struct wifi_manager_t {
  template <typename S>
  static void Connect(S* settings, auto&& print) {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(settings->deviceID() + ".iot");
    WiFi.begin(settings->wifiSSID(), settings->wifiPassword());

    auto wifi_timeout = WIFI_CONNECTION_TIMEOUT / WIFI_CONNECTION_DELAY;

    print("Connecting to ");
    print(settings->wifiSSID());

    while (WiFi.status() != WL_CONNECTED) {
      delay(WIFI_CONNECTION_DELAY);
      print(".");
      if (--wifi_timeout <= 0) {
        print("\r\n");
        print("Error connecting to ");
        print(settings->wifiSSID());
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

  template <typename S>
  static bool SetupAP(S* settings, auto&& print) {
    settings->ensureDeviceID();
    if (!WiFi.softAP(settings->deviceID())) {
      return false;
    }

    print("Access Point ");
    print(settings->deviceID());
    print("\r\n");
    print("IP ");
    print(WiFi.softAPIP().toString());
    print("\r\n");

    return true;
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

  static bool BeginMDNS(const String& hostname) { return MDNS.begin(hostname); }

  static bool BeginMDNS(const String& hostname,
                        std::vector<mdns_service_t> services) {
    auto results = BeginMDNS(hostname);
    if (!results) {
      return results;
    }

    for (const auto& service : services) {
      results = AddMDNSService(service.service, service.proto, service.port);
      if (!results) {
        return false;
      }
    }
    return true;
  }

  static bool AddMDNSService(const String& service,
                             const String& proto,
                             const uint16_t port) {
    return MDNS.addService(service, proto, port);
  }

  static void UpdateMDNS() { MDNS.update(); }

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
