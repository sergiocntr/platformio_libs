
# Mie Librerie Condivise
- curr ver 1.5.9

**Librerie comuni per progetti ESP8266/ESP32 con gestione MQTT e versionamento automatico.**

## 📦 Contenuto
- `uint8_t` Packet Protocol
- Log manager con supporto MQTT e UDP
- EspNow manager
- Gestione WiFi e MQTT
- `version.h` - Definizione versione libreria
- `version_check.h` - Warning automatico versioni
- `library.json` - Metadati per PlatformIO

## 🚀 Utilizzo

Aggiungi il percorso nel `platformio.ini`
```ini
# platformio.ini
lib_deps = 
    https://github.com/sergiocntr/platformio_libs/raw/refs/heads/master/packages/NexManager-1.0.2.tar.gz
```

**Alla compilazione riceverai un warning se la versione non corrisponde.**

***“Un programmatore testardo è qualcuno che quando arriva in un vicolo cieco, invece di tornare indietro, si costruisce una porta...*** 

***e poi magari ci mette anche una maniglia e un campanello.”***