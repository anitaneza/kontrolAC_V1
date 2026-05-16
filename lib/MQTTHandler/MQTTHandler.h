#pragma once

#include <PubSubClient.h>
#include <WiFi.h>

// Tipe callback untuk terima pesan MQTT
typedef void (*MQTTMessageCallback)(const char* topic, const char* payload);

// Callback reconnect MQTT
typedef void (*MQTTReconnectCallback)();


class MQTTHandler {
public:
    MQTTHandler(const char* broker, uint16_t port,
                const char* clientId);

    void connect();
    bool isConnected();
    void loop();

    void publish(const char* topic, float value, int decimal = 2);
    void publish(const char* topic, const char* value);

    void subscribe(const char* topic);
    void setCallback(MQTTMessageCallback cb);

    void setReconnectCallback(MQTTReconnectCallback cb);

private:
    const char* _broker;
    uint16_t    _port;
    const char* _clientId;

    WiFiClient          _wifiClient;
    PubSubClient        _client;
    MQTTMessageCallback _userCallback;

    void reconnect();
    static MQTTHandler* _instance;
    static void _internalCallback(char* topic, byte* payload, unsigned int length);

    MQTTReconnectCallback _reconnectCallback;
};