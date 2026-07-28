#pragma once

class WifiHandler {
public:
    WifiHandler(const char* ssid, const char* password);

    void begin();
    bool isConnected();
    void loop();

private:
    const char* _ssid;
    const char* _password;

    void connect();
    void waitUntilConnected();
};