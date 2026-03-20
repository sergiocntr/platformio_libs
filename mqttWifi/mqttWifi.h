#pragma once
#ifdef ESP8266_BUILD
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#define httpUpdate ESPhttpUpdate

#elif ESP32_BUILD
#include "esp_bt.h"
#include "esp_bt_main.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>
#endif

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <impostazioni_energy.h>
#include <log_lib.h>
#include <myIP.h>
#include <password.h>
#include <topic.h>
// #include "mqttWifiMessages.h"

namespace mqttWifi {
extern PubSubClient client;

// ========== VARIABILI DI STATO ==========
extern IPAddress m_ip;
extern char m_mqtt_id[20];
// Forward declarations
void adessoDormo(uint8_t mode, MotivoSpegnimento motivo);
bool sottoscriviTopic();

// ========== FUNZIONE LOG MOTIVO ==========
void logMotivoSpegnimento(MotivoSpegnimento motivo);

// ========== GESTIONE MODALITÀ RIPOSO ==========
void adessoDormo(uint8_t mode, MotivoSpegnimento motivo);

// ========== SETUP INIZIALE ==========
void setupWifi();

// ========== CONNESSIONE WIFI ==========
bool connectWifi();
// ========== CONNESSIONE MQTT ==========
bool connectMqtt();

// ========== SOTTOSCRIZIONE TOPIC ==========
bool sottoscriviTopics(const char *topics[]);

// ========== GESTIONE PUBBLICAZIONE ==========
bool publish(const char *topic, const char *message, bool retained = false);
bool publish(const char *topic, const uint8_t *payload, size_t length,
             bool retained = false);

// ========== GESTIONE PRINCIPALE (DA CHIAMARE NEL LOOP) ==========
MotivoSpegnimento gestisciConnessione();

// ========== SETUP COMPLETO (DA CHIAMARE IN setup()) ==========
MotivoSpegnimento setupCompleto(IPAddress ip, const char *mqtt_id);

} // namespace mqttWifi