#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

#include <functional>
#include <vector>

#define START_MQTT \
  MQTT->begin();   \
  Serial.println("MQTT started");

template <typename S>
struct mqtt_topic_t;

template <typename S>
struct mqtt_t {
  mqtt_t(Client& client, S* settings, std::vector<mqtt_topic_t<S>> topics)
      : _client(client), _settings(settings), _topics(topics) {}

  void begin() {
    _client.setServer(_settings->mqttServer.c_str(), _settings->mqttPort);
  }

  bool publish(const char* topic, const char* payload) {
    return _client.publish(topic, payload);
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
      for (auto& topic : _topics) {
        topic.publish(this);
      }
      post();
    }

    _client.loop();
  }

 private:
  PubSubClient _client;
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
