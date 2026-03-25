#include "mqttWifiMessages.h"
#include <Arduino.h>
#include <Wire.h>
#include <topic.h>
#include <log_lib.h>

namespace mqttWifi {

  volatile AckState ackReceived = NO_ACK; 

  void setCallback() {
    client.setCallback(callback);
  }

  void callback(char *topic, byte *payload, unsigned int length) {
    if (strcmp(topic, ackMeteoTopic) == 0) {
      String ackMessage = "";
      for (unsigned int i = 0; i < length; i++) {
        ackMessage += (char)payload[i];
      }
      if (strcmp(ackMessage.c_str(), "ack") == 0) ackReceived = ACK;
      else if (strcmp(ackMessage.c_str(), "end") == 0) ackReceived = END;
      else if (strcmp(ackMessage.c_str(), "failed") == 0) ackReceived = FAILED;
      else if (strcmp(ackMessage.c_str(), "error") == 0) ackReceived = ERROR;
    }
  }

  void waitForAck(AckState &ackMsg) {
    unsigned long startMillis = millis();
    while (millis() - startMillis < 1500) {
      client.loop(); 
      delay(10);
      if (ackReceived != NO_ACK) {
        ackMsg = ackReceived;
        ackReceived = NO_ACK; 
        return;
      }
    }
    ackMsg = NO_ACK;
  }

  bool sendPacket(PacketType type, uint8_t *payload, uint16_t payloadLen) {
    // Total size: HEADER_SIZE (5) + Payload + XOR (1)
    uint8_t outBuffer[payloadLen + 10]; 
    size_t frameSize = pp_buildPacket(static_cast<uint8_t>(type), payload, payloadLen, outBuffer);
    
    if (frameSize == 0) return false;
    
    // publish is in mqttWifi library
    return publish(meteoDataTopic, outBuffer, frameSize);
  }

  bool sendDataViaMQTT(uint8_t *rawBuffer, size_t bufferLength) {
    uint8_t nrTry = 0;
    bool sent = false;
    while (!sent && nrTry < 2) {
      sent = sendPacket(TYPE_METEO, rawBuffer, bufferLength);
    if (!sent) {
        LOG_WARN("[MQTT] Pubblicazione fallita, riprovo...");
        nrTry++;
        delay(500);
    }
    }
    return sent;
  }

  bool sendWithRetry(uint8_t *rawBuffer, size_t bufferSize, Eeprom_job &ejob) {
    AckState ackStatus = NO_ACK;
    uint8_t retries = 1; 

    while (retries >= 0) {
      bool sent = sendDataViaMQTT(rawBuffer, bufferSize);
      if (sent) {
        waitForAck(ackStatus);
        if (ackStatus == ACK) { ejob = NO_JOB; return true; }
        else if (ackStatus == FAILED) { 
           LOG_WARN("[ACK] Checksum fallito, riprovo invio...");
           retries--; continue; 
        }
        else if (ackStatus == ERROR) { 
           LOG_ERROR("[ACK] Errore EEPROM su server!");
           ejob = ERASE_EEPROM; return false; 
        }
        else if (ackStatus == END) { ejob = UPDATE_EEPROM; return true; }
        else {
          LOG_ERROR("[ACK] Timeout attesa risposta!");
          ejob = NO_JOB;
          return false;
        }
      } else {
        delay(500);
        retries--;
      }
    }
    ejob = NO_JOB;
    return false;
  }
} // namespace mqttWifi
