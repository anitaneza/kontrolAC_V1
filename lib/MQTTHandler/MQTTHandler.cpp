#include "MQTTHandler.h"
#include <Arduino.h>

MQTTHandler* MQTTHandler::_instance = nullptr;

MQTTHandler::MQTTHandler(const char* broker, uint16_t port,
                         const char* clientId)
    : _broker(broker), _port(port),
      _clientId(clientId),
      _client(_wifiClient),
      _userCallback(nullptr),
      _reconnectCallback(nullptr)
{
    _instance = this;
}

void MQTTHandler::_internalCallback(char* topic, byte* payload, unsigned int length) {
    if (!_instance || !_instance->_userCallback) return;

    char buf[256];
    uint16_t len = min((unsigned int)255, length);
    memcpy(buf, payload, len);
    buf[len] = '\0';

    _instance->_userCallback(topic, buf);
}

void MQTTHandler::setCallback(MQTTMessageCallback cb) {
    _userCallback = cb;
    _client.setCallback(_internalCallback);
}

void MQTTHandler::subscribe(const char* topic, uint8_t qos) {
    _client.subscribe(topic, qos);
    Serial.printf("[MQTT] Subscribe: %s (QoS %d)\n", topic, qos);
}

void MQTTHandler::connect() {
    _client.setServer(_broker, _port);
    reconnect();
}

void MQTTHandler::reconnect() {
    if (_client.connected()) return;

    Serial.print("[MQTT] Menghubungkan ke broker...");
    if (_client.connect(_clientId)) {
        Serial.println(" terhubung.");
        if (_reconnectCallback) _reconnectCallback();
    } else {
        Serial.printf(" gagal, rc=%d\n", _client.state());
    }
}

bool MQTTHandler::isConnected() {
    return _client.connected();
}

void MQTTHandler::loop() {
    if (!_client.connected()) reconnect();
    _client.loop();
}

void MQTTHandler::publish(const char* topic, float value, int decimal, bool retained) {
    char buf[16];
    dtostrf(value, 1, decimal, buf);
    _client.publish(topic, buf, retained);
}

void MQTTHandler::publish(const char* topic, const char* value, bool retained) {
    _client.publish(topic, value, retained);
}

void MQTTHandler::setReconnectCallback(MQTTReconnectCallback cb) {
    _reconnectCallback = cb;
}