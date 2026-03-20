#include "EspNowManager.h"
#include "NexManager.h"

namespace EspNowManager {

bool lastSendSuccess = false;

// Funzione callback per invio (chiamata quando il pacchetto lascia l'ESP)
#if defined(ESP8266)
void onDataSent(uint8_t *mac_addr, uint8_t status) {
  lastSendSuccess = (status == 0); // 0 = Success su ESP8266
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendSuccess = (status == ESP_NOW_SEND_SUCCESS);
#endif
  logSerialPrintf("[ESP-NOW] Last Packet Sent Status: %s\n",
                  lastSendSuccess ? "Delivery Success" : "Delivery Fail");
}

// Funzione callback per ricezione (chiamata quando arriva un pacchetto)
#if defined(ESP32)
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#elif defined(ESP8266)
void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
#endif
  if (len != sizeof(EspPacket)) {
    logSerialPrintf("[ESP-NOW] Received packet with invalid size: %d bytes\n",
                    len);
    return;
  }

  EspPacket packet;
  memcpy(&packet, incomingData, sizeof(EspPacket));

  // Verifica CRC (implementare se necessario)
  // if (packet.crc != calculateCRC(&packet)) { ... }

  logSerialPrintf("[ESP-NOW] Received from MAC: %02X:%02X:%02X:%02X:%02X:%02X "
                  "Type: %d SenderId: %d\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], packet.type,
                  packet.senderId);

  // GESTIONE DEI DATI RICEVUTI
  switch (packet.type) {
  case SENSOR_DATA:
    // In base al senderId, salviamo i dati in 'stato'
    if (packet.senderId == 1) { // Esempio: Terrazza (EXT)
      stato.temps[EXT] = packet.val1;
      stato.hums[EXT] = packet.val2;
      stato.lastUpdate = millis();
    } else if (packet.senderId == 2) { // Esempio: Bagno/Camera (BAG)
      stato.temps[BAG] = packet.val1;
      stato.hums[BAG] = packet.val2;
      stato.lastUpdate = millis();
    }
    NexManager::refreshCurrentPage();
    break;

  case ACK:
    logSerialPrintf("[ESP-NOW] Received ACK from sender: %d\n",
                    packet.senderId);
    break;

  default:
    logSerialPrintln("[ESP-NOW] Unknown packet type");
    break;
  }
}

bool begin() {
#if defined(ESP8266)
  if (esp_now_init() != 0) {
    logSerialPrintln("[ESP-NOW] Error initializing ESP-NOW");
    return false;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Su ESP8266 aggiungiamo il peer (mac, role, channel, key, key_len)
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 12, NULL, 0);
  esp_now_add_peer(gatewayAddress, ESP_NOW_ROLE_COMBO, 12, NULL, 0);

#else // ESP32
  if (esp_now_init() != ESP_OK) {
    logSerialPrintln("[ESP-NOW] Error initializing ESP-NOW");
    return false;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Peer di broadcast
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Peer Gateway
  memcpy(peerInfo.peer_addr, gatewayAddress, 6);
  esp_now_add_peer(&peerInfo);
#endif

  logSerialPrintln("[ESP-NOW] Initialized with SUCCESS");
  return true;
}

bool send(const uint8_t *peerAddress, EspPacket *packet) {
  // Calcolo CRC prima dell'invio
  packet->crc = calculateCRC(packet);
  packet->timestamp = millis();

#if defined(ESP8266)
  int result = esp_now_send((uint8_t *)peerAddress, (uint8_t *)packet,
                            sizeof(EspPacket));
  if (result == 0) {
    return true;
  }
#else
  esp_err_t result =
      esp_now_send(peerAddress, (uint8_t *)packet, sizeof(EspPacket));
  if (result == ESP_OK) {
    return true;
  }
#endif
  logSerialPrintf("[ESP-NOW] Error sending packet: %d\n", result);
  return false;
}

uint16_t calculateCRC(EspPacket *packet) {
  // Semplice checksum XOR degli attributi prima del campo CRC stesso
  uint16_t crc = 0;
  uint8_t *ptr = (uint8_t *)packet;
  // Escludiamo gli ultimi 2 byte (dove risiede il CRC)
  for (size_t i = 0; i < sizeof(EspPacket) - 2; i++) {
    crc ^= ptr[i];
  }
  return crc;
}

} // namespace EspNowManager
