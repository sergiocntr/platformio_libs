#include "udpLogger.h"
#include <stdarg.h>

UdpLogger::UdpLogger() : _port(0), _enabled(true) {}

void UdpLogger::begin(const char* ip, uint16_t port) {
    _ip.fromString(ip);
    _port = port;
    _udp.begin(_port);
}

void UdpLogger::setEnabled(bool enabled) {
    _enabled = enabled;
}

void UdpLogger::send(const char* msg) {
    if (!_enabled) return;

    _udp.beginPacket(_ip, _port);
    _udp.write((const uint8_t*)msg, strlen(msg));
    _udp.endPacket();
}

void UdpLogger::sendf(const char* fmt, ...) {
    if (!_enabled) return;

    char buf[192];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    send(buf);
}