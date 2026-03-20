#include "NexManager.h"
// #include <EspNowManager.h>
#include <impostazioni_chrono.h>

#if defined(ESP32_BUILD)
#define NEX_SERIAL Serial1
#else
#define NEX_SERIAL Serial
#endif
extern void resetTendeTimer();
namespace NexManager {

void begin(unsigned long baud) {
#if defined(ESP32_BUILD)
  NEX_SERIAL.begin(baud, SERIAL_8N1, NEXTION_RX, NEXTION_TX);
#else
  NEX_SERIAL.begin(baud);
#endif
  delay(100);
  sendCommand("");
  LOG_VERBOSE("[NexManager] Initialized at %lu baud\n", baud);
}
void shutdownNextion() {
  delay(100);
  sendCommand("thup=1");
  sendCommand("sleep=1");
  delay(100);
}
void wakeupNextion() {
  sendCommand("dim=20");
  sendCommand("sleep=0");
  delay(100);
}
void sendCommand(const char *cmd) {
  while (NEX_SERIAL.available()) {
    NEX_SERIAL.read();
  }

  NEX_SERIAL.print(cmd);
  NEX_SERIAL.write(0xFF);
  NEX_SERIAL.write(0xFF);
  NEX_SERIAL.write(0xFF);
  NEX_SERIAL.flush();
}

void setText(const char *obj, const char *text) {
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "%s.txt=\"%s\"", obj, text);
  sendCommand(buffer);
}

void setValue(const char *obj, uint32_t value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s.val=%u", obj, value);
  sendCommand(buffer);
}

void setPage(const char *pageId) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "page %s", pageId);
  sendCommand(buffer);
  stato.currPage = atoi(pageId);
  refreshCurrentPage();
}

// Helper per valori float
void setFloat(const char *obj, float value, int decimals = 1) {
  char buffer[32];
  dtostrf(value, 4, decimals, buffer);
  setText(obj, buffer);
}

// Comando generico con formattazione
void sendFormatted(const char *format, ...) {
  char buffer[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  sendCommand(buffer);
}

TouchEvent poll() {
  TouchEvent evt;

  if (NEX_SERIAL.available() < 1)
    return evt;

  uint8_t header = NEX_SERIAL.peek();
  if (header != 0x65 && header != 0x66 && header != 0x70 && header != 0x71) {
    NEX_SERIAL.read(); // scarta byte non riconosciuto
    return evt;
  }

  // ✅ Header valido trovato: aspetta che arrivi il pacchetto completo
  uint8_t required = 0;
  switch (header) {
  case 0x65:
    required = 7;
    break;
  case 0x66:
    required = 5;
    break;
  case 0x71:
    required = 8;
    break;
  case 0x70:
    required = 2;
    break;
  }

  // Aspetta fino a 20ms che arrivino tutti i byte
  uint32_t start = millis();
  while (NEX_SERIAL.available() < required) {
    if (millis() - start > 8) {
      // Timeout: pacchetto incompleto, svuota e rinuncia
      while (NEX_SERIAL.available())
        NEX_SERIAL.read();
      LOG_VERBOSE("[NexTouch] Timeout pacchetto, scartato");
      return evt;
    }
  }

  // Da qui in poi i byte ci sono tutti — procedi come prima
  NEX_SERIAL.read(); // consuma header

  switch (header) {
  case 0x65: {
    evt.page = NEX_SERIAL.read();
    evt.component = NEX_SERIAL.read();
    evt.event = NEX_SERIAL.read();
    for (int i = 0; i < 3; i++)
      NEX_SERIAL.read();
    evt.isValid = true;
    LOG_VERBOSE("[NexTouch] P:%u ID:%u E:%u\n", evt.page, evt.component,
                evt.event);
    break;
  }
  case 0x66: {
    evt.page = NEX_SERIAL.read();
    for (int i = 0; i < 3; i++)
      NEX_SERIAL.read();
    evt.isValid = true;
    LOG_VERBOSE("[NexPage] %u\n", evt.page);
    break;
  }
  case 0x70: {
    while (NEX_SERIAL.available() && NEX_SERIAL.peek() != 0xFF)
      NEX_SERIAL.read();
    for (int i = 0; i < 3; i++)
      if (NEX_SERIAL.available())
        NEX_SERIAL.read();
    break;
  }
  case 0x71: {
    uint32_t val = 0;
    for (int i = 0; i < 4; i++)
      val |= ((uint32_t)NEX_SERIAL.read() << (8 * i));
    for (int i = 0; i < 3; i++)
      NEX_SERIAL.read();
    LOG_VERBOSE("[NexNum] %lu\n", val);
    break;
  }
  default:
    break;
  }

  return evt;
}

// Ripopola il display Nextion dalla struttura stato.
// Chiamare on-demand: al cambio di pagina o al ritorno su una pagina.
void refreshCurrentPage() {

  char buff[16];

  if (stato.currPage == 0) {
    // //LOG_VERBOSE("[refreshCurrentPage] Page %d", stato.currPage);
    // // Raggruppa operazioni simili
    // const char *textWidgets[] = {"Ncurr_hour", "Nday", "Ntcurr", "Nout_temp",
    //                              "Nout_hum", "Nin_hum", "Nwater_temp",
    //                              "Nset_temp"};
    // const char *cropWidgets[] = {"Nrisc_on", "Nwater_on", "Nalarm"};
    // const char *values[] = {stato.timeStr, stato.dayStr, "", "", "", "", "",
    // ""};
    stato.selectionMask =
        0x1F; // Sempre reset a tutte selezionati gli slider tende
    // Aggiorna testo data/ora
    NexManager::sendFormatted("Ncurr_hour.txt=\"%s\"", stato.timeStr);
    NexManager::sendFormatted("Nday.txt=\"%s\"", stato.dayStr);

    // Aggiorna valori float
    dtostrf(stato.temps[INT], 4, 1, buff);
    NexManager::sendFormatted("Ntcurr.txt=\"%s\"", buff);

    dtostrf(stato.temps[EXT], 4, 1, buff);
    NexManager::sendFormatted("Nout_temp.txt=\"%s\"", buff);

    dtostrf(stato.hums[EXT], 4, 1, buff);
    NexManager::sendFormatted("Nout_hum.txt=\"%s\"", buff);

    dtostrf(stato.hums[INT], 4, 1, buff);
    NexManager::sendFormatted("Nin_hum.txt=\"%s\"", buff);

    dtostrf(stato.waterTemp, 4, 1, buff);
    NexManager::sendFormatted("Nwater_temp.txt=\"%s\"", buff);

    // Potenza come intero
    NexManager::sendFormatted("Nset_temp.txt=\"%d\"", stato.powerW);
  } else if (stato.currPage == 1) {
    
    resetTendeTimer(); // <--- Lo mettiamo qui!
    const char *sliderNames[] = {"pl_bar", "tl_bar", "ps_bar", "ts_bar",
                                 "pc_bar"};

    for (int i = 0; i < 5; i++) {
      // La visibilità ora dipende dai bit impostati nella selectionMask
      bool isSelected = (stato.selectionMask & (1 << i));
      NexManager::sendFormatted("vis %s,%d", sliderNames[i], isSelected ? 1 :
      0);

      if (isSelected) {
        NexManager::sendFormatted("%s.val=%d", sliderNames[i], stato.pos[i]);
      }
    }
    NexManager::aggiornaSliderTende();
    stato.selectionMask =
        0x00; // Sempre reset a tutte deselezionate gli slider tende
  }
}

void aggiornaSliderTende() {
  if (stato.currPage != 1)
    return;

  const char *barNames[] = {"ts_bar", "tl_bar", "ps_bar", "pl_bar", "pc_bar"};

  for (int i = 0; i < 5; i++) {
    NexManager::sendFormatted("%s.val=%d", barNames[i], stato.pos[i]);
    LOG_VERBOSE("[Display] Slider tenda %d impostato a %d%%\n", i,
                stato.pos[i]);
    delay(10);
  }
}

} // namespace NexManager
