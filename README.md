# Mie Librerie Condivise
curr ver 1.4.0
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
    https://github.com/sergiocntr/platformio_libs.git#v1.1.1
```

**Alla compilazione riceverai un warning se la versione non corrisponde.**

