#include "EspNowManager.h"

namespace EspNowManager {

// ---------------------------------------------------------------------------
// Strutture interne (private al translation unit)
// ---------------------------------------------------------------------------

namespace {

// --- FIFO di ricezione ------------------------------------------------------

/**
 * Slot del buffer circolare.
 * Copiamo i dati dentro la callback ISR-like; poll() li consuma nel loop.
 */
struct FifoSlot {
  uint8_t mac[6];
  uint8_t data[MAX_PACKET_SIZE];
  uint8_t len;       ///< Lunghezza effettiva (0 = slot libero).
};

FifoSlot  s_fifo[FIFO_DEPTH];
volatile uint8_t s_fifoHead = 0;  ///< Prossimo slot da leggere  (consumer).
volatile uint8_t s_fifoTail = 0;  ///< Prossimo slot da scrivere (producer).

/** Restituisce true se il FIFO è pieno. */
inline bool fifoFull() {
  return ((s_fifoTail + 1u) % FIFO_DEPTH) == s_fifoHead;
}

/** Restituisce true se il FIFO è vuoto. */
inline bool fifoEmpty() {
  return s_fifoHead == s_fifoTail;
}

// --- Lista peer -------------------------------------------------------------

PeerInfo s_peers[MAX_PEERS];
uint8_t  s_peerCount = 0;

/** Cerca un peer per MAC. Restituisce l'indice oppure -1 se non trovato. */
int8_t findPeerIndex(const uint8_t* mac) {
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (s_peers[i].active &&
        memcmp(s_peers[i].mac, mac, 6) == 0) {
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

/** Cerca il primo slot libero. Restituisce l'indice oppure -1. */
int8_t findFreeSlot() {
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (!s_peers[i].active) return static_cast<int8_t>(i);
  }
  return -1;
}

// --- Stato e statistiche ----------------------------------------------------

ReceiveCallback s_receiveCb   = nullptr;
SendCallback    s_sendCb      = nullptr;
bool            s_lastSuccess = false;
Stats           s_stats       = {};

} // namespace anonimo

// ---------------------------------------------------------------------------
// Callback interne ESP-NOW (contesto ISR-like, tenere al minimo)
// ---------------------------------------------------------------------------

#if defined(ESP8266)
void _onDataSent(uint8_t* mac, uint8_t status) {
  s_lastSuccess = (status == 0);
#else
void _onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  s_lastSuccess = (status == ESP_NOW_SEND_SUCCESS);
#endif
  if (s_lastSuccess) {
    s_stats.txDelivered++;
  }
  if (s_sendCb) {
    s_sendCb(mac, s_lastSuccess);
  }
}

#if defined(ESP32)
void _onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
#elif defined(ESP8266)
void _onDataRecv(uint8_t* mac, uint8_t* data, uint8_t len) {
#endif
  s_stats.rxTotal++;

  // Scarta se dimensione non valida
  if (!mac || !data || len <= 0 || len > MAX_PACKET_SIZE) {
    s_stats.rxDropped++;
    return;
  }

  // Scarta se FIFO pieno
  if (fifoFull()) {
    s_stats.rxDropped++;
    return;
  }

  // Scrivi nello slot corrente e avanza tail
  FifoSlot& slot = s_fifo[s_fifoTail];
  memcpy(slot.mac,  mac,  6);
  memcpy(slot.data, data, static_cast<size_t>(len));
  slot.len = static_cast<uint8_t>(len);

  s_fifoTail = (s_fifoTail + 1u) % FIFO_DEPTH;
}

// ---------------------------------------------------------------------------
// Configurazione
// ---------------------------------------------------------------------------

void setReceiveCallback(ReceiveCallback cb) { s_receiveCb = cb; }
void setSendCallback(SendCallback cb)        { s_sendCb    = cb; }

// ---------------------------------------------------------------------------
// Gestione peer
// ---------------------------------------------------------------------------

bool addPeer(const uint8_t* mac, uint8_t channel) {
  if (!mac) return false;

  // Già presente: consideralo successo
  if (findPeerIndex(mac) >= 0) return true;

  // Lista interna piena
  int8_t slot = findFreeSlot();
  if (slot < 0) return false;

#if defined(ESP8266)
  int res = esp_now_add_peer(
    const_cast<uint8_t*>(mac),
    ESP_NOW_ROLE_COMBO,
    channel,
    nullptr,
    0
  );
  if (res != 0) return false;

#else // ESP32
  // Rimuovi prima per evitare ESP_ERR_ESPNOW_EXIST
  esp_now_del_peer(mac);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = channel;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) return false;
#endif

  // Salva nella lista interna
  memcpy(s_peers[slot].mac, mac, 6);
  s_peers[slot].channel = channel;
  s_peers[slot].active  = true;
  s_peerCount++;
  return true;
}

bool removePeer(const uint8_t* mac) {
  if (!mac) return false;

  int8_t idx = findPeerIndex(mac);
  if (idx < 0) return false;

#if defined(ESP8266)
  if (esp_now_del_peer(const_cast<uint8_t*>(mac)) != 0) return false;
#else
  if (esp_now_del_peer(mac) != ESP_OK) return false;
#endif

  s_peers[idx].active = false;
  s_peerCount--;
  return true;
}

bool hasPeer(const uint8_t* mac) {
  return mac && (findPeerIndex(mac) >= 0);
}

uint8_t peerCount() {
  return s_peerCount;
}

void getPeers(PeerInfo* out, uint8_t& outCount) {
  outCount = 0;
  if (!out) return;
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (s_peers[i].active) {
      out[outCount++] = s_peers[i];
    }
  }
}

// ---------------------------------------------------------------------------
// Ciclo di vita
// ---------------------------------------------------------------------------

bool begin() {
  // Azzera strutture interne
  memset(s_fifo,  0, sizeof(s_fifo));
  memset(s_peers, 0, sizeof(s_peers));
  s_fifoHead   = 0;
  s_fifoTail   = 0;
  s_peerCount  = 0;
  s_lastSuccess = false;
  s_stats       = {};

#if defined(ESP8266)
  if (esp_now_init() != 0) return false;
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(_onDataSent);
  esp_now_register_recv_cb(_onDataRecv);

#else // ESP32
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(_onDataSent);
  esp_now_register_recv_cb(_onDataRecv);
#endif

  return true;
}

void end() {
  // Rimuovi tutti i peer dallo stack
  for (uint8_t i = 0; i < MAX_PEERS; i++) {
    if (s_peers[i].active) {
#if defined(ESP8266)
      esp_now_del_peer(s_peers[i].mac);
#else
      esp_now_del_peer(s_peers[i].mac);
#endif
      s_peers[i].active = false;
    }
  }
  s_peerCount = 0;

  esp_now_unregister_send_cb();
  esp_now_unregister_recv_cb();
  esp_now_deinit();

  // Svuota il FIFO
  s_fifoHead = s_fifoTail = 0;

  s_receiveCb = nullptr;
  s_sendCb    = nullptr;
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------

uint8_t poll() {
  uint8_t processed = 0;

  while (!fifoEmpty() && s_receiveCb) {
    // Leggi dallo slot corrente
    FifoSlot& slot = s_fifo[s_fifoHead];

    // Avanza head PRIMA di chiamare la callback
    // (così la callback può chiamare send() senza deadlock)
    uint8_t mac[6];
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t len = slot.len;

    memcpy(mac,  slot.mac,  6);
    memcpy(data, slot.data, len);
    slot.len = 0;

    s_fifoHead = (s_fifoHead + 1u) % FIFO_DEPTH;

    s_receiveCb(mac, data, len);
    processed++;
  }

  return processed;
}

// ---------------------------------------------------------------------------
// Trasmissione
// ---------------------------------------------------------------------------

bool send(const uint8_t* mac, const uint8_t* data, size_t len) {
  if (!mac || !data || len == 0 || len > ESPNOW_MAX_LEN) {
    s_stats.txFailed++;
    return false;
  }

#if defined(ESP8266)
  int res = esp_now_send(
    const_cast<uint8_t*>(mac),
    const_cast<uint8_t*>(data),
    static_cast<uint8_t>(len)
  );
  bool ok = (res == 0);
#else
  bool ok = (esp_now_send(mac, data, len) == ESP_OK);
#endif

  if (ok) {
    s_stats.txTotal++;
  } else {
    s_stats.txFailed++;
  }
  return ok;
}

// ---------------------------------------------------------------------------
// Diagnostica
// ---------------------------------------------------------------------------

bool lastSendSucceeded() { return s_lastSuccess; }

Stats getStats()    { return s_stats; }
void  resetStats()  { s_stats = {}; }

} // namespace EspNowManager