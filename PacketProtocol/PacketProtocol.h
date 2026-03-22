#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <devices.h>
/**
 * @file PacketProtocol.h
 * @brief Standardized communication buffer protocol.
 *
 * Design principles
 * ─────────────────
 * • PacketType  →  describes the PAYLOAD FORMAT (which struct to cast to).
 * • deviceID    →  first byte of every payload; identifies WHO sent the frame.
 *
 * This separation means adding a new room/node only requires a new deviceID
 * in devices.h — the parser and structs stay unchanged.
 *
 * Frame layout
 * ────────────
 *   [0]      Magic          0xAA
 *   [1]      Version        0x01
 *   [2]      Type           PacketType  ← HOW to read the payload
 *   [3..4]   PayloadLength  uint16_t, little-endian
 *   [5]      deviceID       first byte of payload  ← WHO sent it
 *   [6..N]   rest of payload
 *   [N+1]    XOR checksum   over bytes [0..N]
 *
 * Total frame size = HEADER_SIZE + payloadLength + 1
 *
 * Encoding conventions — ALL sensor fields are uint16_t
 * ───────────────────────────────────────────────────────
 *   temperature  →  (float + 50.0) × 128    range  -50 … +462 °C
 *   humidity     →   float         × 128    range    0 … 100 %
 *   pressure     →   float         × 16     range    0 … 4095 hPa
 *   battery      →   mV, 1:1                unsigned, no scaling
 *   blind pos    →   0-100 %                uint8_t, no scaling
 *
 * Encode (sender side, C)
 * ────────────────────────
 *   d.temperature = PP_ENCODE_TEMP(temp);   // (temp + 50) * 128
 *   d.humidity    = PP_ENCODE_HUM(hum);     // hum * 128
 *   d.pressure    = PP_ENCODE_PRESS(press); // press * 16
 *
 * Decode (receiver side, C)
 * ──────────────────────────
 *   float t = PP_DECODE_TEMP(d->temperature);  // val/128.0 - 50.0
 *   float h = PP_DECODE_HUM(d->humidity);      // val/128.0
 *   float p = PP_DECODE_PRESS(d->pressure);    // val/16.0
 *
 * Decode (Node-RED / JavaScript)
 * ────────────────────────────────
 *   temp:     payload.readUInt16LE(offset) / 128 - 50
 *   humidity: payload.readUInt16LE(offset) / 128
 *   pressure: payload.readUInt16LE(offset) / 16
 *   blind:    payload.readUInt8(offset)           // already 0-100
 */

// ── Protocol constants ────────────────────────────────────────────────────────
#define PACKET_MAGIC    0xAA
#define PACKET_VERSION  0x01
#define HEADER_SIZE     5       // bytes before payload
#define PACKET_MIN_SIZE 6       // smallest legal frame

// ── Encoding helpers ──────────────────────────────────────────────────────────
#define PP_TEMP_OFFSET  50.0f
#define PP_TEMP_SCALE   128.0f
#define PP_HUM_SCALE    128.0f
#define PP_PRESS_SCALE  16.0f

#define PP_ENCODE_TEMP(t)  ((uint16_t)(((t) + PP_TEMP_OFFSET) * PP_TEMP_SCALE))
#define PP_DECODE_TEMP(v)  ((v) / PP_TEMP_SCALE - PP_TEMP_OFFSET)
#define PP_ENCODE_HUM(h)   ((uint16_t)((h) * PP_HUM_SCALE))
#define PP_DECODE_HUM(v)   ((v) / PP_HUM_SCALE)
#define PP_ENCODE_PRESS(p) ((uint16_t)((p) * PP_PRESS_SCALE))
#define PP_DECODE_PRESS(v) ((v) / PP_PRESS_SCALE)

// ── Packet types (= payload format) ──────────────────────────────────────────
typedef enum {
    TYPE_STATUS  = 0x01,   // generic status  (future use)
    TYPE_COMMAND = 0x02,   // command frame   (future use)
    TYPE_DHT     = 0x03,   // DHT22   → struct dhtData
    TYPE_DS18    = 0x04,   // DS18B20 → struct ds18Data
    TYPE_BME     = 0x05,   // BME280  → struct bmeData
    TYPE_PZEM    = 0x06,   // PZEM    → struct EneMainData
    TYPE_TENDE   = 0x07,   // Blinds  → struct tendeData
    TYPE_UNKNOWN = 0xFF
} PacketType;

// ── Payload structures ────────────────────────────────────────────────────────

#pragma pack(push, 1)
/**
 * TYPE_DHT — DHT22 room sensor (salotto, camera, …)
 * Size: 1 + 2 + 2 + 1 = 6 bytes
 */
struct dhtData {
    uint8_t  deviceID;    // vedi file devices.h
    uint16_t humidity;    // PP_ENCODE_HUM(float)
    uint16_t temperature; // PP_ENCODE_TEMP(float)
    uint8_t  comfort;     // bitmask: bit0=OK 1=TooHot 2=TooCold 3=TooDry 4=TooHumid
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * TYPE_DS18 — DS18B20 temperature-only sensor (bagno, …)
 * humidity is always 0 — kept for layout symmetry with dhtData.
 * Size: 1 + 2 + 2 + 1 = 6 bytes
 */
struct ds18Data {
    uint8_t  deviceID;    // DEV_BAGNO, …
    uint16_t humidity;    // always 0 (no humidity sensor)
    uint16_t temperature; // PP_ENCODE_TEMP(float)
    uint8_t  comfort;     // computed from temperature only
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * TYPE_BME — BME280 weather station (Mariner, …)
 * Size: 1 + 2 + 2 + 2 + 2 = 9 bytes
 */
struct bmeData {
    uint8_t  deviceID;    // DEV_MARINER, …
    uint16_t humidity;    // PP_ENCODE_HUM(float)
    uint16_t temperature; // PP_ENCODE_TEMP(float)
    uint16_t pressure;    // PP_ENCODE_PRESS(float)   hPa
    uint16_t battery;     // mV, raw (1:1, no scaling)
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * TYPE_PZEM — energy meter
 * Size: 1 + 2 + 2 + 2 + 2 = 9 bytes
 */
struct EneMainData {
    uint8_t  deviceID; // DEV_PZEM, …
    uint16_t v;        // voltage  float × 16
    uint16_t i;        // current  float × 128
    uint16_t c;        // cos φ    float × 128
    uint16_t e;        // power    W (raw)
};
#pragma pack(pop)

#pragma pack(push, 1)
/**
 * TYPE_TENDE — Tuya blind positions (Node-RED → subscribers)
 * Size: 1 + 5 = 6 bytes
 *
 * pos[] is indexed by blind id (0-based).
 * numBlinds = header.payloadLength - 1  (excludes deviceID byte)
 *
 * Decode:
 *   const tendeData *d = (const tendeData *)pkt.payload;
 *   uint8_t n = pkt.header.payloadLength - 1;
 *   for (int i = 0; i < n; i++) { use d->pos[i]; }
 */
struct tendeData {
    uint8_t deviceID;   // DEV_TUYA_TENDE
    uint8_t pos[5];     // pos[i] = blind i opening, 0-100 %
};
#pragma pack(pop)

// ── Frame header ──────────────────────────────────────────────────────────────
#pragma pack(push, 1)
/** Fixed 5-byte header preceding every payload. */
struct StandardHeader {
    uint8_t  magic;          // PACKET_MAGIC   (0xAA)
    uint8_t  version;        // PACKET_VERSION (0x01)
    uint8_t  type;           // PacketType  ← HOW
    uint16_t payloadLength;  // little-endian
};
#pragma pack(pop)

// ── Parsed-packet result ──────────────────────────────────────────────────────
/**
 * @brief Output of pp_parsePacket().
 *
 * On success:
 *   header   — decoded 5-byte header.
 *   payload  — pointer into the caller's buffer (zero-copy; do not free).
 *
 * Usage:
 *   ParsedPacket pkt;
 *   if (pp_parsePacket(buf, len, &pkt) == 0) {
 *       switch (pkt.header.type) {
 *
 *           case TYPE_DHT: {
 *               const dhtData *d = (const dhtData *)pkt.payload;
 *               float t = PP_DECODE_TEMP(d->temperature);
 *               float h = PP_DECODE_HUM(d->humidity);
 *               break;
 *           }
 *           case TYPE_DS18: {
 *               const ds18Data *d = (const ds18Data *)pkt.payload;
 *               float t = PP_DECODE_TEMP(d->temperature);
 *               break;
 *           }
 *           case TYPE_BME: {
 *               const bmeData *d = (const bmeData *)pkt.payload;
 *               float t = PP_DECODE_TEMP(d->temperature);
 *               float p = PP_DECODE_PRESS(d->pressure);
 *               break;
 *           }
 *           case TYPE_TENDE: {
 *               const tendeData *d = (const tendeData *)pkt.payload;
 *               uint8_t n = pkt.header.payloadLength - 1;
 *               for (int i = 0; i < n; i++) {
 *                   // d->pos[i] = position of blind i (0-100%)
 *               }
 *               break;
 *           }
 *       }
 *   }
 */
typedef struct {
    StandardHeader  header;
    const uint8_t  *payload;  // points into original buffer — do not free
} ParsedPacket;

// ── Public API ────────────────────────────────────────────────────────────────
#ifdef __cplusplus
extern "C" {
#endif

/** XOR over every byte in data[0..length-1]. */
uint8_t pp_calculateXOR(const uint8_t *data, size_t length);

/**
 * Build a complete frame into outBuffer.
 * outBuffer must be at least (payloadLen + 6) bytes.
 * Returns total frame size, or 0 on error.
 */
size_t pp_buildPacket(uint8_t type,
                      const uint8_t *payload, uint16_t payloadLen,
                      uint8_t *outBuffer);

/**
 * Validate a received frame (magic + size + XOR checksum).
 * Returns  0  if valid.
 *         -1  NULL pointer or frame too small.
 *         -2  bad magic word.
 *         -3  bufferSize does not match declared payload length.
 *         -4  XOR checksum mismatch.
 */
int pp_validatePacket(const uint8_t *buffer, size_t bufferSize);

/**
 * Validate + decode a received frame into *out.
 * out->payload points into buffer (zero-copy).
 * Returns same codes as pp_validatePacket(), plus:
 *         -5  out is NULL.
 */
int pp_parsePacket(const uint8_t *buffer, size_t bufferSize, ParsedPacket *out);

#ifdef __cplusplus
}
#endif

#endif // PACKET_PROTOCOL_H