#include "PacketProtocol.h"
#include <string.h>

/**
 * @file PacketProtocol.cpp
 * @brief Implementation for the standardized communication buffer protocol.
 */

// --- XOR Implementation ---
uint8_t pp_calculateXOR(const uint8_t *data, size_t length) {
    uint8_t result = 0;
    for (size_t i = 0; i < length; i++) {
        result ^= data[i];
    }
    return result;
}

// --- Frame Construction ---
size_t pp_buildPacket(uint8_t type, const uint8_t *payload, uint16_t payloadLen, uint8_t *outBuffer) {
    if (outBuffer == NULL) return 0;

    // 1. Build Header (5 bytes)
    outBuffer[0] = PACKET_MAGIC;
    outBuffer[1] = PACKET_VERSION;
    outBuffer[2] = type;
    outBuffer[3] = (uint8_t)(payloadLen & 0xFF);         // LSB
    outBuffer[4] = (uint8_t)((payloadLen >> 8) & 0xFF);  // MSB

    // 2. Add Payload
    if (payload != NULL && payloadLen > 0) {
        memcpy(outBuffer + 5, payload, payloadLen);
    }

    // 3. Calculate Checksum (Header + Payload)
    uint8_t checksum = pp_calculateXOR(outBuffer, 5 + payloadLen);
    
    // 4. Record Checksum (1 byte)
    outBuffer[5 + payloadLen] = checksum;

    return 5 + payloadLen + 1;
}

// --- Frame Validation ---
int pp_validatePacket(const uint8_t *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize < 6) return -1; // Minimum header + 1 check byte

    // 1. Check Magic Word
    if (buffer[0] != PACKET_MAGIC) return -2;

    // 2. Extract Length
    uint16_t payloadLen = (uint16_t)buffer[3] | ((uint16_t)buffer[4] << 8);
    
    // 3. Size check (Total bytes = 5 + payloadLen + 1)
    if (bufferSize != (size_t)(5 + payloadLen + 1)) return -3;

    // 4. Cross-check XOR (all bytes of header + payload)
    uint8_t calculatedXOR = pp_calculateXOR(buffer, 5 + payloadLen);
    if (buffer[5 + payloadLen] != calculatedXOR) return -4;

    return 0; // Success
}
