#pragma once
// Forza l'inclusione degli header corretti per ESP8266

#include <Arduino.h>


// Includi esplicitamente c_types per ESP8266
#ifdef ESP8266
#include <c_types.h>
#endif

/**
 * @namespace NexManager
 * @brief Gestione leggera e non bloccante della comunicazione con il display
 * Nextion.
 * // Invece di setPicture, setText, setValue...
    NexManager::sendFormatted("%s.picc=%d", "c0", 5);           // Crop picture
    NexManager::sendFormatted("%s.txt=\"%s\"", "t0", "Ciao"); // Testo
    NexManager::sendFormatted("%s.val=%d", "h0", 50);         // Valore numerico
    NexManager::sendFormatted("page %d", 1);                   // Cambio pagina
    NexManager::sendFormatted("vis %s,%d", "q0", 1);           // Visibilità
    NexManager::sendFormatted("click %s,%d", "b0", 1);         // Simula click

    // Per valori float (come facevi con dtostrf)
    char buff[16];
    dtostrf(temperatura, 4, 1, buff);
    NexManager::sendFormatted("%s.txt=\"%s\"", "t_temp", buff);
 */
namespace NexManager {

struct TouchEvent {
    uint8_t page;
    uint8_t component;
    uint8_t event;  // 0=release, 1=press
    bool isValid;
    
    TouchEvent() : page(0), component(0), event(0), isValid(false) {}
};
/* // Invece di setPicture, setText, setValue...
    NexManager::sendFormatted("%s.picc=%d", "c0", 5);           // Crop picture
    NexManager::sendFormatted("%s.txt=\"%s\"", "t0", "Ciao"); // Testo
    NexManager::sendFormatted("%s.val=%d", "h0", 50);         // Valore numerico
    NexManager::sendFormatted("page %d", 1);                   // Cambio pagina
    NexManager::sendFormatted("vis %s,%d", "q0", 1);           // Visibilità
    NexManager::sendFormatted("click %s,%d", "b0", 1);         // Simula click

    // Per valori float (come facevi con dtostrf)
    char buff[16];
    dtostrf(temperatura, 4, 1, buff);
    NexManager::sendFormatted("%s.txt=\"%s\"", "t_temp", buff);
 */
void sendFormatted(const char *format, ...); 

/**
 * @brief Inizializza la comunicazione seriale con il Nextion.
 * @param baud Velocità di comunicazione (consigliata 38400 o higher).
 */
void begin(unsigned long baud = 38400);

/**
 * @brief Invia un comando raw al Nextion seguito dai tre byte 0xFF.
 * @param cmd Stringa del comando.
 */
void sendCommand(const char *cmd);

/**
 * @brief Funzione di polling da chiamare nel loop per intercettare i dati dal
 * Nextion. Gestisce touch event, risposte a "get", ecc.
 */
TouchEvent poll();

/**
 * @brief Forza l'aggiornamento di tutti i widget della pagina corrente
 * usando i dati contenuti nella struct 'stato'.
 */
void refreshCurrentPage();
void shutdownNextion();
void wakeupNextion();
void setPage(const char *pageId);
void aggiornaSliderTende();

} // namespace NexManager
