## VERSIONE 1.4.0
- Aggiunto library.json per ogni libreria
- Aggiunta cartella `shared_config1 per dichiarazioni condivise
- altri piccoli fix


## VERSIONE 1.2.0
### Modifica nella libreria ESPManager
- Rimosse tutte le dipendenze esterne — niente impostazioni_chrono.h, NexManager.h, stato, EspPacket, broadcastAddress, gatewayAddress. 
 - La libreria non sa nulla dell'applicazione (agnostica).
**FIFO circolare (ring buffer)**
- La callback ISR-like di ESP-NOW scrive nello slot tail e incrementa il puntatore — nient'altro. poll() legge da head, copia i dati su stack locale, avanza head prima di chiamare la tua callback, così la callback stessa può chiamare send() senza problemi.
**Gestione peer multipli**
- Array statico di PeerInfo[20] (limite hardware ESP-NOW). Le funzioni addPeer / removePeer / hasPeer / peerCount / getPeers mantengono sincronizzati la lista interna e lo stack ESP-NOW.
**Statistiche**
- La struct Stats traccia rxTotal, rxDropped, txTotal, txFailed, txDelivered — utile per diagnosticare perdite di pacchetti o problemi di consegna senza Serial.print sparsi.

**Nota**

La libreria è pulita e dovrebbe integrarsi senza problemi. 
Quando la provi, tieni d'occhio i rxDropped nelle statistiche — se salgono significa che il FIFO si sta riempiendo e potresti aumentare FIFO_DEPTH o chiamare poll() più frequentemente nel loop.

**Esempio d'uso**

```cpp
EspNowManager::setReceiveCallback([](const uint8_t* mac,
                                     const uint8_t* data,
                                     size_t len) {
  // decodifica qui la tua struttura
});

EspNowManager::begin();
EspNowManager::addPeer(mac1);
EspNowManager::addPeer(mac2);

// Nel loop():
EspNowManager::poll();
```
## VERSIONE 1.1.0
**Gestione modulare delle dipendenze**
  - Deploy su github delle librerie condivise
  - version.h con definizioni di versione
  - version_check.h con logica di warning
  - Macro EXPECTED_LIB_VERSION_MAJOR/MINOR nei progetti

**Sottoscrizione dinamica MQTT**
  - Funzione sottoscriviTopics() con array di topic
  - Terminatore nullptr per flessibilità