#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>

class UdpLogger {
public:
    UdpLogger();

    void begin(const char* ip, uint16_t port);
    void send(const char* msg);
    void sendf(const char* fmt, ...);

    void setEnabled(bool enabled);

private:
    WiFiUDP _udp;
    IPAddress _ip;
    uint16_t _port;
    bool _enabled;
};