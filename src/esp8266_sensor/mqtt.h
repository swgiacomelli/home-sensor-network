#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <PubSubClient.h>

#include <functional>
#include <vector>

#if defined(SECURE_MQTT)
#include "mqtt_ssl.h"
#include "wifi.h"
#endif

#define START_MQTT \
  MQTT->begin();   \
  Serial.println("MQTT started");

template <typename S>
struct mqtt_topic_t;

template <typename S>
struct mqtt_t {
  mqtt_t(S* settings, std::vector<mqtt_topic_t<S>> topics)
      : _client(_network_client), _settings(settings), _topics(topics) {
#if defined(SECURE_MQTT)
    _clientCertList = new BearSSL::X509List(mqtt::client_cert);
    _clientPrivKey = new BearSSL::PrivateKey(mqtt::client_key);
    _trustedCA = new BearSSL::X509List(mqtt::server_ca);
    _network_client.setClientRSACert(_clientCertList, _clientPrivKey);
    _network_client.setTrustAnchors(_trustedCA);
    _network_client.setInsecure();  // this is insecure
#endif
  }

  void begin() {
    _client.setServer(_settings->mqttServer.c_str(), _settings->mqttPort);
  }

  bool publish(const char* topic, const char* payload) {
    return _client.publish(topic, payload);
  }

  bool publishState(bool state) {
    String topic = "sensors/" + _settings->deviceID + "/state";
    return _client.publish(topic.c_str(), state ? "1" : "0");
  }

  S* getSettings() { return _settings; }

  void loop() {
    loop([]() {}, []() {});
  }

  void loop(auto&& pre, auto&& post) {
    if (!_client.connected()) {
      _client.connect(_settings->deviceID.c_str(),
                      _settings->mqttUsername.c_str(),
                      _settings->mqttPassword.c_str());
    }

    if (_client.connected()) {
      pre();
      bool results = true;
      for (auto& topic : _topics) {
        if (!topic.publish(this)) {
          results = false;
        }
      }
      publishState(!_topics.empty() && results);
      post();
    }

    _client.loop();
  }

 private:
  PubSubClient _client;
#if defined(SECURE_MQTT)
  BearSSL::WiFiClientSecure _network_client;
  BearSSL::X509List* _clientCertList;
  BearSSL::PrivateKey* _clientPrivKey;
  BearSSL::X509List* _trustedCA;
#else
  WiFiClient _network_client;
#endif
  S* _settings;
  std::vector<mqtt_topic_t<S>> _topics;
};

template <typename S>
struct mqtt_topic_t {
  friend struct mqtt_t<S>;
  using ReadFunction = std::function<double(void)>;

  mqtt_topic_t(const String& topicName,
               ReadFunction&& read,
               uint8_t precision = 2)
      : _topicName(topicName), _read(read), _precision(precision) {}

 protected:
  bool publish(mqtt_t<S>* mqtt) {
    String value = String(_read(), _precision);
    String topic =
        "sensors/" + mqtt->getSettings()->deviceID + "/" + _topicName;
    return mqtt->publish(topic.c_str(), value.c_str());
  }

 private:
  String _topicName;
  ReadFunction _read;
  uint8_t _precision;
};
