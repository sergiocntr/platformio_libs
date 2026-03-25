#include "mqttWifi.h"
#include "IPAddress.h"
#include "PubSubClient.h"
#include "log_lib.h"
#include "mqttWifi_transport.h"
#include "topic.h"

WiFiClient mywifi;
WiFiClient c;

static IMqttTransport *mqttTransport = nullptr;
static MqttTransportType currentTransport = DEFAULT_MQTT_TRANSPORT;

namespace mqttWifi {
PubSubClient client(c);

void setMqttTransport(MqttTransportType t) {
  if (mqttTransport) {
    mqttTransport->disconnect();
    delete mqttTransport;
  }
  currentTransport = t;
  mqttTransport = createMqttTransport(t);
  if (mqttTransport)
    mqttTransport->init();
}

MqttTransportType getMqttTransport() { return currentTransport; }

// ========== CONFIGURAZIONE ==========
const uint8_t MAX_TENTATIVI = 3;
const unsigned long TIMEOUT_WIFI = 12000;
const unsigned long TIMEOUT_MQTT = 12000;
const unsigned long SLEEP_TIME_US = 5ULL * 60ULL * 1000000ULL;

uint8_t tentativiWifi = 0;
uint8_t tentativiMqtt = 0;
IPAddress m_ip;
char m_mqtt_id[20];
const char **m_topics = nullptr; // array topic fornito dal progetto
// ========== GESTIONE PUBBLICAZIONE ==========
bool publish(const char *topic, const char *message, bool retained) {
  if (!client.connected()) {
    LOG_ERROR("[PUBLISH] Client non connesso");
    return false;
  }

  // Tentativi multipli con delay ridotto
  for (size_t tentativo = 0; tentativo < 3; tentativo++) {
    client.loop(); // Mantieni connessione attiva

    // publish(topic, payload, retained)
    if (client.publish(topic, message, retained)) {
      LOG_VERBOSE("[PUBLISH] OK su tentativo %d %s\n", tentativo + 1,
                  retained ? "(RETAINED)" : "");
      return true; // Successo
    }

    LOG_ERROR("[PUBLISH] Fallito tentativo %d/3\n", tentativo + 1);
    delay(50); // Breve pausa tra tentativi
  }

  // Dopo 3 tentativi falliti, entra in modalità riposo
  adessoDormo(8, PUBLISH_FALLITO);
  return false;
}

bool publish(const char *topic, const uint8_t *payload, size_t length,
             bool retained) {
  if (!client.connected()) {
    LOG_ERROR("[PUBLISH BIN] Client non connesso");
    return false;
  }

  for (size_t tentativo = 0; tentativo < 3; tentativo++) {
    client.loop(); // Mantieni connessione attiva

    if (client.publish(topic, payload, length, retained)) {
      LOG_VERBOSE("[PUBLISH BIN] OK su tentativo %d %s\n", tentativo + 1,
                  retained ? "(RETAINED)" : "");
      return true; // Successo
    }

    LOG_ERROR("[PUBLISH BIN] Fallito tentativo %d/3\n", tentativo + 1);
    delay(50); // Breve pausa tra tentativi
  }

  // Dopo 3 tentativi falliti, entra in modalità riposo
  adessoDormo(8, PUBLISH_FALLITO);
  return false;
}

// ========== LOG MOTIVO SPEGNIMENTO ==========
void logMotivoSpegnimento(MotivoSpegnimento motivo) {
  LOG_INFO("[SLEEP] Motivo: ");
  switch (motivo) {
  case CLEAN_SHUTDOWN:
    LOG_INFO("CLEAN SHUTDOWN");
    break;
  case PUBLISH_FALLITO:
    LOG_ERROR("PUBLISH FALLITO dopo 3 tentativi");
    break;
  case COMANDO_SYSTEM_TOPIC:
    LOG_VERBOSE("COMANDO via systemTopic (payload '0')");
    break;
  case WIFI_TIMEOUT_CONNESSIONE:
    LOG_ERROR("WiFi TIMEOUT dopo 3 tentativi");
    break;
  case MQTT_TIMEOUT_CONNESSIONE:
    LOG_ERROR("MQTT TIMEOUT dopo 3 tentativi");
    break;
  case WIFI_FALLITO_SETUP:
    LOG_ERROR("WiFi FALLITO durante setup");
    break;
  case NEXTION_SETUP_FAILED:
    LOG_ERROR("NEXTION INIT FAILLITO durante setup");
    break;
  case DHT_SETUP_FAILED:
    LOG_ERROR("DHT INIT FAILLITO durante setup");
    break;
  case SHUTDOWN_FROM_MQTT:
    LOG_INFO("SHUTDOWN FROM MQTT");
    break;
  case ONLY_DISCONNETS:
    LOG_INFO("ONLY DISCONNECTS: non entro in deep sleep");
    break;
  default:
    LOG_WARN("SCONOSCIUTO");
    break;
  }
  Serial.flush();
}

// ========== DEEP SLEEP ==========
void adessoDormo(uint8_t mode, MotivoSpegnimento motivo) {
  logMotivoSpegnimento(motivo);

  // Spegnimento Nextion
  if (mode > 0) {
    LOG_VERBOSE("[SLEEP] Spegnimento Nextion");
    // sendCommand("thup=1");  // gestito altrove
    // sendCommand("sleep=1");
    delay(200);
  }

  // Chiusura MQTT
  if (client.connected()) {
    LOG_VERBOSE("[SLEEP] Disconnessione MQTT");
    client.disconnect();
    delay(100);
  }

  // Spegnimento WiFi
  LOG_VERBOSE("[SLEEP] Spegnimento WiFi");
  delay(50);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(200);

  if (MotivoSpegnimento::ONLY_DISCONNETS == motivo) {
    LOG_VERBOSE("[SLEEP] ONLY DISCONNECTS: non entro in deep sleep");
    return;
  }

  // Deep sleep
  LOG_VERBOSE("[SLEEP] Deep sleep per 5 minuti");
  Serial.flush();
  delay(100);
#ifdef ESP8266_BUILD
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.forceSleepBegin();
  delay(300000);
  ESP.reset();

#elif ESP32_BUILD
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  esp_deep_sleep_start(); // NON ritorna mai
#endif
}

// ========== SETUP WiFi ==========
void setupWifi() {
  WiFi.persistent(false);
  WiFi.disconnect(true);

  WiFi.mode(WIFI_OFF);
  delay(200);

  WiFi.mode(WIFI_STA);
  delay(200);

  WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
#ifdef ESP8266_BUILD
  WiFi.setOutputPower(17);
  WiFi.forceSleepWake();
  LOG_VERBOSE("[WiFi] Setup ESP8266");

#elif ESP32_BUILD
  LOG_VERBOSE("[WiFi] Setup ESP32 C3");
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

#endif

  WiFi.config(m_ip, gateway, subnet, dns1);
}

void randomDelayAtBoot() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  unsigned long delayMs = ((mac[4] + mac[5]) % 100) * 10;
  LOG_VERBOSE("[SETUP] Delay casuale: %lu ms\n", delayMs);
  delay(delayMs);
}

// ========== CONNESSIONE WiFi CON RETRY ==========
bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    tentativiWifi = 0; // Reset contatore se connesso
    return true;
  }

  LOG_VERBOSE("[WiFi] Tentativo %d/%d\n", tentativiWifi + 1, MAX_TENTATIVI);
  WiFi.begin(ssid, password);
  delay(200);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > TIMEOUT_WIFI) {
      LOG_WARN("[WiFi] TIMEOUT");
      tentativiWifi++;
      return false;
    }
    delay(250);
    // logSerialPrint(".");
  }

  LOG_VERBOSE("\n[WiFi] ✓ Connesso a %s\n", WiFi.localIP().toString());

  tentativiWifi = 0; // Reset contatore
  delay(50);
  return true;
}

// ========== CONNESSIONE MQTT CON RETRY ==========
bool connectMqtt() {
  if (client.connected()) {
    tentativiMqtt = 0; // Reset contatore se connesso
    return true;
  }

  LOG_VERBOSE("[MQTT] Tentativo %d/%d\n", tentativiMqtt + 1, MAX_TENTATIVI);

  String clientId = String(m_mqtt_id) + String(random(0xffff), HEX);

  client.setServer(ipMqtt_server.toString().c_str(), mqtt_port);
  client.setBufferSize(512);

  uint32_t start = millis();
  while (!client.connected()) {
    if (millis() - start > TIMEOUT_MQTT) {
      LOG_WARN("[MQTT] TIMEOUT");
      tentativiMqtt++;
      return false;
    }

    if (client.connect(clientId.c_str(), mqttUser, mqttPass)) {
      LOG_VERBOSE("[MQTT] ✓ Connesso");
      tentativiMqtt = 0; // Reset contatore
      return sottoscriviTopics(m_topics);
    }

    delay(250);
    // logSerialPrint(".");
  }

  return false;
}

// ========== SOTTOSCRIZIONE TOPIC ==========
bool sottoscriviTopics(const char *topics[]) {
  LOG_VERBOSE("[MQTT] Sottoscrizione topic dinamici...");

  char buffer[50];
  sprintf(buffer, "Dispositivo connesso v. %d", versione);
  client.publish(logTopic, buffer);
  delay(10);

  bool success = true;
  int successCount = 0;
  int totalTopics = 0;

  // Unico ciclo: sottoscrivi e conta allo stesso tempo
  for (int i = 0; topics[i] != nullptr; i++) {
    bool subscribed = client.subscribe(topics[i]);

    if (subscribed) {
      successCount++;
      LOG_VERBOSE("[MQTT] ✓ Sottoscritto: %s", topics[i]);
    } else {
      success = false;
      LOG_ERROR("[MQTT] ✗ ERRORE CRITICO: %s", topics[i]);
    }

    totalTopics++;
    client.loop();
    delay(5);
  }

  if (success) {
    LOG_VERBOSE("[MQTT] ✓ Sottoscrizioni completate: %d/%d", successCount,
                totalTopics);
  } else {
    LOG_ERROR("[MQTT] ✗ Sottoscrizioni fallite: %d/%d", successCount,
              totalTopics);
  }

  return success;
}

MotivoSpegnimento gestisciConnessione() {
  // ── WiFi ──────────────────────────────────────────────
  while (WiFi.status() != WL_CONNECTED) // ✅ while prima dell'if
  {
    LOG_WARN("[GESTIONE] WiFi disconnesso, tentativo %d/%d\n",
             tentativiWifi + 1, MAX_TENTATIVI);

    if (connectWifi()) {
      LOG_INFO("[GESTIONE] WiFi connesso dopo %d tentativi\n",
               tentativiWifi + 1);
      tentativiWifi = 0;
      break; // ✅ connesso, esci dal while
    }

    tentativiWifi++;
    if (tentativiWifi >= MAX_TENTATIVI) {
      LOG_ERROR("[GESTIONE] WiFi fallito dopo %d tentativi\n", MAX_TENTATIVI);
      tentativiWifi = 0;
      return WIFI_TIMEOUT_CONNESSIONE;
    }

    delay(500); // ✅ aspetta prima del prossimo tentativo
  }

  // ── MQTT ──────────────────────────────────────────────
  while (!client.connected()) // ✅ stesso pattern del WiFi
  {
    LOG_WARN("[GESTIONE] MQTT disconnesso, tentativo %d/%d\n",
             tentativiMqtt + 1, MAX_TENTATIVI);

    if (connectMqtt()) {
      LOG_INFO("[GESTIONE] MQTT connesso dopo %d tentativi\n",
               tentativiMqtt + 1); // ✅ fix: era tentativiWifi
      tentativiMqtt = 0;
      break;
    }

    tentativiMqtt++;
    if (tentativiMqtt >= MAX_TENTATIVI) {
      LOG_ERROR("[GESTIONE] MQTT fallito dopo %d tentativi\n", MAX_TENTATIVI);
      tentativiMqtt = 0;
      return MQTT_TIMEOUT_CONNESSIONE;
    }

    delay(1000);
  }

  // ── Tutto OK ──────────────────────────────────────────
  client.loop();
  return CONN_OK;
}

// ========== SETUP COMPLETO ==========
MotivoSpegnimento setupCompleto(IPAddress ip, const char *mqtt_id,
                                const char *topics[]) {
  m_ip = ip;
  m_topics = topics;
  strcpy(m_mqtt_id, mqtt_id);

  if (!mqttTransport) {
    setMqttTransport(DEFAULT_MQTT_TRANSPORT);
  }

  setupWifi();   // 1️⃣ WiFi prima
  udpLogBegin(); // 2️⃣ poi inizializza UDP log
  LOG_VERBOSE("========================================");
  LOG_VERBOSE("[SETUP] Avvio mqttWifi");
  LOG_VERBOSE("========================================");

  randomDelayAtBoot();

  return gestisciConnessione();
}

} // namespace mqttWifi