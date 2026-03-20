#pragma once
#include <Arduino.h>
#include <impostazioni_chrono.h>

#if defined(ESP32)
#include <WiFi.h>
#include <esp_now.h>
#elif defined(ESP8266)
//#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

/**
 * @namespace EspNowManager
 * @brief Gestione della comunicazione ESP-NOW bidirezionale.
 */
namespace EspNowManager {

/**
 * @brief Inizializza ESP-NOW e registra le callback.
 * @return true se inizializzato con successo.
 */
bool begin();

extern bool lastSendSuccess;

/**
 * @brief Invia un pacchetto a un indirizzo MAC specifico o in broadcast.
 * @param peerAddress Indirizzo MAC del destinatario (6 byte).
 * @param packet Puntatore al pacchetto EspPacket da inviare.
 * @return true se l'invio è stato preso in carico.
 */
bool send(const uint8_t *peerAddress, EspPacket *packet);

/**
 * @brief Calcola il CRC per un pacchetto (semplice XOR o altro).
 * @param packet Puntatore al pacchetto.
 * @return Valore CRC.
 */
uint16_t calculateCRC(EspPacket *packet);

/**
 * @brief Callback interna chiamata alla ricezione di un pacchetto.
 */
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

/**
 * @brief Callback interna chiamata dopo l'invio di un pacchetto.
 */
#if defined(ESP8266)
void onDataSent(uint8_t *mac_addr, uint8_t status);
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
#endif

} // namespace EspNowManager
