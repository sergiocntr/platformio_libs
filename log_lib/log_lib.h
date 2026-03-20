#ifndef ENERGY_LOG_H
#define ENERGY_LOG_H

#include <Arduino.h>

#ifdef ESP8266_BUILD
#include <ESP8266WiFi.h>
#elif ESP32_BUILD
#include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include <stdarg.h>

// ============================================================
//  Configurazione Debug e Logger
// ============================================================
#define DEBUG_CHRONO
#define DEBUG_UDP_LOG // UDP sempre attivo
#define DEBUG_LEVEL 3 // 0=solo errori, 1=warning, 2=info, 3=verbose

#define UDP_LOG_IP "192.168.1.100" // IP del PC che riceve i log
#define UDP_LOG_PORT 4444

#ifdef DEBUG_UDP_LOG
extern WiFiUDP udpLog;

void udpLogBegin();
void udpLogSend(const char *msg);
void udpLogSend_f(const char *fmt, ...);

// Helper per convertire qualsiasi tipo, incluso IPAddress
template <typename T> inline String _toStr(T val) { return String(val); }
inline String _toStr(IPAddress ip) { return ip.toString(); }

#define LOG_ERROR(...) udpLogSend_f(__VA_ARGS__) // sempre attivo

#if DEBUG_LEVEL >= 1
#define LOG_WARN(...) udpLogSend_f(__VA_ARGS__)
#else
#define LOG_WARN(...) do {} while (0)
#endif

#if DEBUG_LEVEL >= 2
#define LOG_INFO(...) udpLogSend_f(__VA_ARGS__)
#else
#define LOG_INFO(...) do {} while (0)
#endif

#if DEBUG_LEVEL >= 3
#define LOG_VERBOSE(...) udpLogSend_f(__VA_ARGS__)
#else
#define LOG_VERBOSE(...) do {} while (0)
#endif

#elif defined(DEBUG_CHRONO)
// Seriale classica
#define logSerial Serial
#define logSerialBegin(a) logSerial.begin(a)
#define logSerialPrint(a) logSerial.print(a)
#define LOG_ERROR(...) logSerial.printf(__VA_ARGS__)
#define LOG_WARN(...) logSerial.printf(__VA_ARGS__)
#define LOG_INFO(...) logSerial.printf(__VA_ARGS__)
#define LOG_VERBOSE(...) logSerial.printf(__VA_ARGS__)

#else
// Produzione — tutto sparisce
#define logSerialBegin(a) do {} while (0)
#define logSerialPrint(a) do {} while (0)
#define LOG_ERROR(...) do {} while (0)
#define LOG_WARN(...) do {} while (0)
#define LOG_INFO(...) do {} while (0)
#define LOG_VERBOSE(...) do {} while (0)
#endif

// ========== ENUM MOTIVI SPEGNIMENTO ==========
enum MotivoSpegnimento {
  PUBLISH_FALLITO = 0,
  COMANDO_SYSTEM_TOPIC = 1,
  WIFI_TIMEOUT_CONNESSIONE = 2,
  MQTT_TIMEOUT_CONNESSIONE = 3,
  WIFI_FALLITO_SETUP = 4,
  MQTT_FALLITO_RISVEGLIO = 5,
  WIFI_FALLITO_RISVEGLIO = 6,
  NEXTION_SETUP_FAILED = 7,
  DHT_SETUP_FAILED = 8,
  SETUP_OK = 253,
  CONN_OK = 254,
  SHUTDOWN_FROM_MQTT = 255
};
extern MotivoSpegnimento m_wifi_status;
enum RelayIdx {
  RISCALDAMENTO = 0,
  ACQUA = 1,
  ALLARME = 2,
  CAMERA = 3,
  TERRAZZA = 4,
  CALDAIA = 5,
  ENERGIA = 6,
  MAX_RELAY = 7
};
#endif // ENERGY_LOG_H
