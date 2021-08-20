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
// config.h defines the configuration server.

#pragma once

#include <Arduino.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "config_html.h"
#include "config_ssl.h"

#include "wifi.h"

#include "json.h"

namespace config {
static const char html_mime_type[] PROGMEM = "text/html";
static const char css_mime_type[] PROGMEM = "text/css";
static const char js_mime_type[] PROGMEM = "application/javascript";
static const char json_mime_type[] PROGMEM = "application/json";

static const char header_connection[] PROGMEM = "Connection";
static const char header_connection_close[] PROGMEM = "close";

static const char update_success_message[] PROGMEM =
    R"({"success":true, "message":"Device successfully updated - going to sensor mode."})";
static const char update_failed_message[] PROGMEM =
    R"({"success":false, "message":"Error configuring device - check settings."})";
}  // namespace config

template <typename S>
struct configuration_server_t {
  configuration_server_t(S* settings)
      : _server_cache(5),
        _server(443),
        _settings(settings),
        _configured(false) {
    _server.getServer().setRSACert(new BearSSL::X509List(config::server_cert),
                                   new BearSSL::PrivateKey(config::server_key));
    _server.getServer().setCache(&_server_cache);

    _server.on("/", HTTP_GET, [&]() {
      _server.sendHeader(config::header_connection,
                         config::header_connection_close);
      _server.send_P(200, config::html_mime_type, config::index_html);
    });

    _server.on("/config.css", HTTP_GET, [&]() {
      _server.sendHeader(config::header_connection,
                         config::header_connection_close);
      _server.send_P(200, config::css_mime_type, config::config_css);
    });

    _server.on("/config.js", HTTP_GET, [&]() {
      _server.sendHeader(config::header_connection,
                         config::header_connection_close);
      _server.send_P(200, config::js_mime_type, config::config_js);
    });

    _server.on("/values", HTTP_GET, [&]() { on_values(); });

    _server.on("/networks", HTTP_GET, [&]() { on_networks(); });

    _server.on("/update", HTTP_POST, [&]() { on_update(); });

    _server.onNotFound([&]() { on_not_found(); });
    _server.begin();

    MDNS.begin(_settings->deviceID());
    MDNS.addService("https", "tcp", 443);
  }

  bool isConfigured() { return _configured; }

  static void Run(S* settings, auto&& print) {
    auto server = configuration_server_t(settings);

    print("Running configuration\r\n");

    settings->ensureDeviceID();
    WiFi.softAP(settings->deviceID());

    print("Access Point " + settings->deviceID() + "\r\n");
    print("IP " + WiFi.softAPIP().toString() + "\r\n");

    while (!server.isConfigured()) {
      server.loop();
      yield();
    }

    delay(10000);
    ESP.reset();
  }

 private:
  void loop() {
    _server.handleClient();
    MDNS.update();
  }

  void on_networks() {
    auto network_list = json_t::as_json_array(wifi_manager_t::ScanNetworks());
    _server.sendHeader(config::header_connection,
                       config::header_connection_close);
    _server.send(200, config::json_mime_type, network_list);
  }

  void on_values() {
    auto values = _settings->to_json();
    _server.sendHeader(config::header_connection,
                       config::header_connection_close);
    _server.send(200, config::json_mime_type, values);
  }

  void on_update() {
    auto client = _server.client();

    if (_server.hasArg("plain")) {
      auto values = _server.arg("plain");
      _settings->from_json(values);
    }

    _server.sendHeader(config::header_connection,
                       config::header_connection_close);

    if (_settings->validate()) {
      _settings->save();
      _server.send_P(200, config::json_mime_type,
                     config::update_success_message);
      _configured = true;
    } else {
      _server.send_P(200, config::json_mime_type,
                     config::update_failed_message);
    }
  }

  void on_not_found() {
    String message = "File Not Found\n";
    message += "URI: ";
    message += _server.uri();
    message += "\n";
    _server.sendHeader(config::header_connection,
                       config::header_connection_close);
    _server.send(404, "text/plain", message);
  }

  BearSSL::ESP8266WebServerSecure _server;
  BearSSL::ServerSessions _server_cache;

  S* _settings;
  bool _configured;
};