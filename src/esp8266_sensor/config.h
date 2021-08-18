#pragma once

#include <Arduino.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <TZ.h>

#include <NoDelay.h>

#include "config_ssl.h"

namespace config {
static const char html_mime_type[] PROGMEM = "text/html";
static const char css_mime_type[] PROGMEM = "text/css";
static const char index_html[] PROGMEM =
    R"(<!DOCTYPE html><html><head> <meta charset='utf-8'> <meta http-equiv='X-UA-Compatible' content='IE=edge'> <title>Device Configuration</title> <meta name='viewport' content='width=device-width, initial-scale=1'> <link rel="stylesheet" href="/config.min.css"></head><body> <form method='POST' action='/update' enctype='multipart/form-data'> <h1>Device Configuration</h1> <fieldset> <legend>General</legend> <div><label for="deviceID">Device ID: <abbr title="required" aria-label="required">*</abbr></label><input type="text" name="deviceID" id="deviceID" autofocus="true" required><span></span> </div></fieldset> <fieldset> <legend>WIFI</legend> <div><label for="wifiSSID">SSID: <abbr title="required" aria-label="required">*</abbr></label><input type="text" name="wifiSSID" id="wifiSSID" list="wifiNetworks" required><span></span><datalist id="wifiNetworks"> </datalist></div><div><label for="wifiPassword">Password: <abbr title="required" aria-label="required">*</abbr></label><input type="password" name="wifiPassword" id="wifiPassword" required><span></span> </div></fieldset> <fieldset> <legend>MQTT</legend> <div><label for="mqttServer">Server:<abbr title="required" aria-label="required">*</abbr></label><input type="text" name="mqttServer" id="mqttServer" required><span></span></div><div><label for="mqttPort">Port: <abbr title="required" aria-label="required">*</abbr></label><input type="number" name="mqttPort" id="mqttPort" value="1883" required><span></span></div><div><label for="mqttUsername">Username:</label><input type="text" name="mqttUsername" id="mqttUsername" placeholder="Blank For Anonymous"></div><div><label for="mqttPassword">Password: </label><input type="password" name="mqttPassword" id="mqttPassword" placeholder="Blank For Anonymous"></div></fieldset> <div class="button"><button type="submit">Update</button></div></form> <script src="/config.min.js"></script></body></html>)";
static const char config_min_css[] PROGMEM =
    R"(body {font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;font-size: small;}h1 {margin: 0;text-align: center;}form {margin: 0 auto;width: 500px;padding: 1em;border: 1px solid #CCC;border-radius: 1em;}form div+div {margin-top: 1em;}label {display: inline-block;width: 90px;text-align: right;margin-right: 1em;}legend {font-weight: bold;}input {font-size: 1em;width: 300px;box-sizing: border-box;border: 1px solid #999;box-shadow: inset 1px 1px 3px #ccc;border-radius: 5px;}input:required {border: 1px solid black;}input:optional {border: 1px solid silver;}input:focus {border-color: #000;}input+span {position: relative;}input+span::before {position: absolute;right: -20px;top: 5px;}input:invalid {border: 2px solid red;}input:invalid+span::before {content: '✖';color: red;}input:valid+span::before {content: '✓';color: green;}.button {margin-top: 1em;display: flex;justify-content: flex-end;}button {margin-left: .5em;display: block;box-sizing: border-box;border-radius: 5px;background: linear-gradient(to bottom, #eee, #ccc);}button:hover {background: linear-gradient(to bottom, #fff, #ddd);})";
static const char config_min_js[] PROGMEM =
    R"(document.getElementById("deviceID").setAttribute("value",deviceID),document.getElementById("wifiNetworks").innerHTML=networks.map(e=>"<option>"+e+"</option>").join("");)";

static const char update_html[] PROGMEM =
    R"(<!DOCTYPE html><html><head> <meta charset='utf-8'> <meta http-equiv='X-UA-Compatible' content='IE=edge'> <title>Device Configuration</title> <meta name='viewport' content='width=device-width, initial-scale=1'> <link rel="stylesheet" href="/update.min.css"></head><body> <div> <h1>Device Configuration</h1> <p>Configuration Successful</p><p>Device Going to Sensor Mode</p></div></body></html>)";
static const char update_min_css[] PROGMEM =
    R"(body {font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;font-size: small;}h1 {margin: 0;}div {margin: 0 auto;width: 500px;padding: 1em;border: 1px solid #CCC;border-radius: 1em;text-align: center;})";
}  // namespace config

template <typename S>
struct configuration_server_t {
  configuration_server_t(S* settings)
      : _server_cache(5), _server(443), _settings(settings), _configured(false) {
    _server.getServer().setRSACert(new BearSSL::X509List(config::server_cert),
                                   new BearSSL::PrivateKey(config::server_key));
    _server.getServer().setCache(&_server_cache);

    _server.on("/", [&]() {
      _server.send_P(200, config::html_mime_type, config::index_html);
    });
    _server.on("/config.min.css", [&]() {
      _server.send_P(200, config::css_mime_type, config::config_min_css);
    });
    _server.on("/config.min.js", [&]() { onConfigJS(); });
    _server.on("/update.min.css", [&]() {
      _server.send_P(200, config::css_mime_type, config::update_min_css);
    });
    _server.on("/update", HTTP_POST, [&]() { onUpdate(); });
    _server.onNotFound([&]() { onNotFound(); });
    _server.begin();

    MDNS.begin(_settings->deviceID);
    MDNS.addService("https", "tcp", 443);
  }

  bool isConfigured() { return _configured; }

  static void Run(S* settings, auto&& println) {
    auto server = configuration_server_t(settings);
    println("Running configuration");
    settings->ensureDeviceID();
    WiFi.softAP(settings->deviceID);
    println("Access Point " + settings->deviceID);
    println("IP " + WiFi.softAPIP().toString());

    configTime(TZ_Etc_UTC, "pool.ntp.org");

    while (!server.isConfigured()) {
      server.loop();
      yield();
    }

    noDelay resetDelay(60000);
    resetDelay.start();
    while (!resetDelay.update()) {
      server.loop();
      yield();
    }

    ESP.reset();
  }

 private:
  void loop() { _server.handleClient(); }
  void onConfigJS() {
    String message;
    message += "var deviceID=\"";
    message += _settings->deviceID;
    message += "\";var networks=[";

    auto scanResult = WiFi.scanNetworks(false, false);
    for (int8_t i = 0; i < scanResult; i++) {
      String ssid;
      int32_t rssi;
      uint8_t encryptionType;
      uint8_t* bssid;
      int32_t channel;
      bool hidden;
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel,
                          hidden);
      message += "\"" + ssid + "\"";
      if (i != scanResult - 1) {
        message += ",";
      }
    }
    message += "];";
    message += config::config_min_js;
    _server.send(200, "application/javascript", message);
  }

  void onUpdate() {
    _settings->deviceID = readArg("deviceID");

    _settings->wifiSSID = readArg("wifiSSID");
    _settings->wifiPassword = readArg("wifiPassword");

    _settings->mqttServer = readArg("mqttServer");
    _settings->mqttPort = (uint16_t)readArg("mqttPort").toInt();
    _settings->mqttUsername = readArg("mqttUsername");
    _settings->mqttPassword = readArg("mqttPassword");

    _settings->trim();

    if (_settings->validate()) {
      _settings->save();
      _server.send_P(200, config::html_mime_type, config::update_html);
      _configured = true;
    } else {
      _server.sendHeader("Location", "/");
      _server.send(303, "text/plain",
                   "Configuration Errors\nReturning to Configuration");
    }
  }

  void onNotFound() {
    String message = "File Not Found\n";
    message += "URI: ";
    message += _server.uri();
    message += "\n";
    _server.send(404, "text/plain", message);
  }

  String readArg(const String& name) {
    if (_server.hasArg(name)) {
      auto value = _server.arg(name);
      value.trim();
      return value;
    }
    return {};
  }

  BearSSL::ESP8266WebServerSecure _server;
  BearSSL::ServerSessions _server_cache;

  S* _settings;
  bool _configured;
};