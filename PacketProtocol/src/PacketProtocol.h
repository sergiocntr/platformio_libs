#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file PacketProtocol.h
 * @brief Standardized communication buffer protocol definitions.
 * 
 * This library defines the hardware-agnostic frame structure for
 * serial or network communications.
 */

// Protocol Constants
#define PACKET_MAGIC    0xAA
#define PACKET_VERSION  0x01
#define HEADER_SIZE     5

// Packet Types
typedef enum {
    TYPE_STATUS  = 0x01,
    TYPE_COMMAND = 0x02,
    TYPE_METEO   = 0x03,
    TYPE_CHRONO  = 0x04,
    TYPE_BAGNO   = 0x05,
    TYPE_PZEM    = 0x06,
    TYPE_UNKNOWN = 0xFF
} PacketType;
#pragma pack(push, 1)
/**
 * @brief Structure for weather data telemetry.
 */
struct meteoData {
  uint8_t deviceID;         // Unique device identifier
  int16_t humidityBMP;      // Humidity (float * 128)
  int16_t temperatureBMP;   // Temperature (float * 128)
  int16_t externalPressure; // Atmospheric pressure (float * 128)
  uint16_t battery;         // Battery voltage in mV
  uint16_t moisture;        // Soil moisture raw ADC value
  uint8_t padding[3];       // 3-byte padding to maintain 16-byte total size
  uint8_t counter;          // Record sequence counter
  uint8_t checksum;         // Integrity check byte
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * @brief Payload structure for Chrono project.
 */
struct chronoData {
    uint16_t humidity;      // Humidity (float * 128)
    uint16_t temperature;   // Temperature ((float + 50) * 128)
    uint8_t comfort;       // Comfort index
};
#pragma pack(pop)
#pragma pack(push, 1)
struct EneMainData{
  uint16_t v; //volt (float * 16)
  uint16_t i;  //current (float * 128)
  uint16_t c;  //cospi (float * 128)
  uint16_t e; //current power
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * @brief Standard 5-byte header structure.
 * Byte order: MAGIC, VERSION, TYPE, LENGTH_LSB, LENGTH_MSB
 */
struct StandardHeader {
    uint8_t magic;          // Fixed MAGIC word (0xAA)
    uint8_t version;        // Protocol Version (0x01)
    uint8_t type;           // Data Type (e.g., 0x03 for Meteo)
    uint16_t payloadLength; // Length of the payload (Little Endian)
};
#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculates the XOR checksum for a given buffer.
 */
uint8_t pp_calculateXOR(const uint8_t *data, size_t length);

/**
 * @brief Wraps a payload into a standardized frame.
 * 
 * @param type The packet type (PacketType enum)
 * @param payload Pointer to the payload data
 * @param payloadLen Length of the payload
 * @param outBuffer Destination buffer (must be at least payloadLen + 6 bytes)
 * @return Total size of the final packet (header + payload + checksum)
 */
size_t pp_buildPacket(uint8_t type, const uint8_t *payload, uint16_t payloadLen, uint8_t *outBuffer);

/**
 * @brief Validates a packet by checking its header and checksum.
 * 
 * @param buffer The packet buffer to check
 * @param bufferSize Total size of the buffer
 * @return 0 if valid, non-zero if invalid
 */
int pp_validatePacket(const uint8_t *buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // PACKET_PROTOCOL_H
