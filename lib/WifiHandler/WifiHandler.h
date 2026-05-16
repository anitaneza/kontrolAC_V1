#pragma once

class WifiHandler {
public:
    WifiHandler(const char* ssid, const char* password);

    void connect();
    bool isConnected();

private:
    const char* _ssid;
    const char* _password;

    void waitUntilConnected();
};