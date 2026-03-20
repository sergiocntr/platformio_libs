#include "log_lib.h"
#include <WiFiUdp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG_UDP_LOG
WiFiUDP udpLog;

void udpLogBegin() { udpLog.begin(UDP_LOG_PORT); }

void udpLogSend(const char *msg) {
  udpLog.beginPacket(UDP_LOG_IP, UDP_LOG_PORT);
  udpLog.write((uint8_t *)msg, strlen(msg));
  udpLog.endPacket();
}

void udpLogSend_f(const char *fmt, ...) {
  if (m_wifi_status != CONN_OK)
    return;
  char buf[192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  udpLogSend(buf);
}
#endif
