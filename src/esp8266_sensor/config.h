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

#include "web_server_impl.h"

#include "config_html.h"

#include "wifi.h"

#include "json.h"

namespace config {
static const char update_success_message[] PROGMEM =
    R"({"success":true, "message":"Device successfully updated - going to sensor mode."})";
static const char update_failed_message[] PROGMEM =
    R"({"success":false, "message":"Error configuring device - check settings."})";
}  // namespace config

template <typename S>
struct configuration_server_t {
  configuration_server_t() = delete;

  configuration_server_t(S* settings)
      : _web_server(
            {
                {"/", HTTP_GET,
                 [](web_server_t& server) {
                   server.send_html_P(200, config::index_html);
                 }},
                {"/config.css", HTTP_GET,
                 [](web_server_t& server) {
                   server.send_css_P(200, config::config_css);
                 }},
                {"/config.js", HTTP_GET,
                 [](web_server_t& server) {
                   server.send_js_P(200, config::config_js);
                 }},
                {"/networks", HTTP_GET,
                 [](web_server_t& server) {
                   auto network_list =
                       json_t::as_json_array(wifi_manager_t::ScanNetworks());
                   server.send_json(200, network_list);
                 }},
                {"/values", HTTP_GET,
                 [&](web_server_t& server) {
                   auto values = _settings->to_json();
                   server.send_json(200, values);
                 }},
                {"/update", HTTP_POST,
                 [&](web_server_t& server) { on_update(server); }},
            },
            web_server_t::on_not_found),
        _settings(settings),
        _configured(false) {
    _web_server.begin();
    wifi_manager_t::BeginMDNS(settings->deviceID(), {{"https", "tcp", 443}});
  }

  bool isConfigured() { return _configured; }

  static void Run(S* settings, auto&& print) {
    auto server = configuration_server_t(settings);

    print("Running configuration\r\n");

    if (!wifi_manager_t::SetupAP(settings, print)) {
      print("Failed to setup access point.\r\n");
      ESP.deepSleep(0);
    }

    while (!server.isConfigured()) {
      server.loop();
      yield();
    }

    delay(10000);
    ESP.reset();
  }

 private:
  void loop() {
    _web_server.loop();
    wifi_manager_t::UpdateMDNS();
  }

  void on_update(web_server_t& server) {
    auto values = server.request_body();
    _settings->from_json(values);

    if (_settings->validate()) {
      _settings->save();
      server.send_json_P(200, config::update_success_message);
      _configured = true;
    } else {
      server.send_json_P(200, config::update_failed_message);
    }
  }

  web_server_t _web_server;
  S* _settings;
  bool _configured;
};