# Packet Protocol — Reference Card

## Frame layout

```
┌─────────┬─────────┬─────────┬──────────────┬──────────────────────────┬─────────┐
│ byte 0  │ byte 1  │ byte 2  │  bytes 3-4   │      bytes 5 … N         │ byte N+1│
├─────────┼─────────┼─────────┼──────────────┼──────────────────────────┼─────────┤
│  Magic  │ Version │  Type   │    Length    │         Payload          │   XOR   │
│  0xAA   │  0x01   │PacketType│ uint16 LE   │  deviceID + data bytes   │ 1 byte  │
└─────────┴─────────┴─────────┴──────────────┴──────────────────────────┴─────────┘
                        │                              │
                        │ HOW to read the payload      │ WHO sent the frame
                        ▼                              ▼
                    TYPE_*                          DEV_*
```

- **Header**: 5 byte fissi
- **Payload**: `payloadLength` byte (dichiarato in byte 3-4, little-endian)
- **XOR**: calcolato su tutti i byte da 0 a N (header + payload, escluso se stesso)
- **Frame totale**: `5 + payloadLength + 1` byte

---

## Encoding conventions

Tutti i campi sensor sono `uint16_t` senza segno con offset/scala fissi:

| Grandezza   | Encode (sender)              | Decode (receiver C)                  | Decode (Node-RED JS)                        |
|-------------|------------------------------|--------------------------------------|---------------------------------------------|
| temperature | `(float + 50.0) × 128`       | `val / 128.0f - 50.0f`               | `payload.readUInt16LE(off) / 128 - 50`      |
| humidity    | `float × 128`                | `val / 128.0f`                       | `payload.readUInt16LE(off) / 128`           |
| pressure    | `float × 16`                 | `val / 16.0f`                        | `payload.readUInt16LE(off) / 16`            |
| battery     | `mV` (raw, 1:1)              | `val` (mV)                           | `payload.readUInt16LE(off)`                 |
| blind pos   | `0-100` (raw, 1:1)           | `val` (%)                            | `payload.readUInt8(off)`                    |

**Macro C disponibili** (in `PacketProtocol.h`):
```c
PP_ENCODE_TEMP(t)   PP_DECODE_TEMP(v)
PP_ENCODE_HUM(h)    PP_DECODE_HUM(v)
PP_ENCODE_PRESS(p)  PP_DECODE_PRESS(v)
```

---

## TYPE → struct (HOW)

### `TYPE_DHT = 0x03` → `struct dhtData` — DHT22

```
┌──────────┬──────────┬──────────┬──────────┐
│ byte 0   │ byte 1-2 │ byte 3-4 │  byte 5  │
│ deviceID │ humidity │   temp   │ comfort  │
│ uint8    │ uint16LE │ uint16LE │  uint8   │
└──────────┴──────────┴──────────┴──────────┘
 Payload = 6 byte    Frame totale = 12 byte
```

| Campo      | Tipo      | Decode                        |
|------------|-----------|-------------------------------|
| deviceID   | uint8     | vedi tabella DEV_*            |
| humidity   | uint16 LE | `val / 128.0`  → %            |
| temperature| uint16 LE | `val / 128.0 - 50` → °C       |
| comfort    | uint8     | bitmask: bit0=OK 1=Hot 2=Cold 3=Dry 4=Humid |

```javascript
// Node-RED decode TYPE_DHT
const deviceID = payload.readUInt8(5);
const hum      = payload.readUInt16LE(6) / 128;
const temp     = payload.readUInt16LE(8) / 128 - 50;
const comfort  = payload.readUInt8(10);
```

---

### `TYPE_DS18 = 0x04` → `struct ds18Data` — DS18B20

```
┌──────────┬──────────┬──────────┬──────────┐
│ byte 0   │ byte 1-2 │ byte 3-4 │  byte 5  │
│ deviceID │ humidity │   temp   │ comfort  │
│ uint8    │ uint16LE │ uint16LE │  uint8   │
│          │ sempre 0 │          │          │
└──────────┴──────────┴──────────┴──────────┘
 Payload = 6 byte    Frame totale = 12 byte
```

Layout identico a `dhtData` — `humidity` è sempre 0 (DS18B20 non ha igrometro).

```javascript
// Node-RED decode TYPE_DS18
const deviceID = payload.readUInt8(5);
const temp     = payload.readUInt16LE(8) / 128 - 50;
const comfort  = payload.readUInt8(10);
```

---

### `TYPE_BME = 0x05` → `struct bmeData` — BME280

```
┌──────────┬──────────┬──────────┬──────────┬──────────┐
│ byte 0   │ byte 1-2 │ byte 3-4 │ byte 5-6 │ byte 7-8 │
│ deviceID │ humidity │   temp   │ pressure │ battery  │
│ uint8    │ uint16LE │ uint16LE │ uint16LE │ uint16LE │
└──────────┴──────────┴──────────┴──────────┴──────────┘
 Payload = 9 byte    Frame totale = 15 byte
```

| Campo      | Tipo      | Decode                        |
|------------|-----------|-------------------------------|
| deviceID   | uint8     | vedi tabella DEV_*            |
| humidity   | uint16 LE | `val / 128.0`  → %            |
| temperature| uint16 LE | `val / 128.0 - 50` → °C       |
| pressure   | uint16 LE | `val / 16.0`  → hPa           |
| battery    | uint16 LE | `val` → mV                    |

```javascript
// Node-RED decode TYPE_BME
const deviceID = payload.readUInt8(5);
const hum      = payload.readUInt16LE(6)  / 128;
const temp     = payload.readUInt16LE(8)  / 128 - 50;
const press    = payload.readUInt16LE(10) / 16;
const battery  = payload.readUInt16LE(12);
```

---

### `TYPE_PZEM = 0x06` → `struct EneMainData` — PZEM energy meter

```
┌──────────┬──────────┬──────────┬──────────┬──────────┐
│ byte 0   │ byte 1-2 │ byte 3-4 │ byte 5-6 │ byte 7-8 │
│ deviceID │    V     │    I     │   cosφ   │    W     │
│ uint8    │ uint16LE │ uint16LE │ uint16LE │ uint16LE │
└──────────┴──────────┴──────────┴──────────┴──────────┘
 Payload = 9 byte    Frame totale = 15 byte
```

| Campo    | Tipo      | Decode              |
|----------|-----------|---------------------|
| deviceID | uint8     | vedi tabella DEV_*  |
| v        | uint16 LE | `val / 16.0`  → V   |
| i        | uint16 LE | `val / 128.0` → A   |
| c        | uint16 LE | `val / 128.0` → cosφ|
| e        | uint16 LE | `val` → W           |

```javascript
// Node-RED decode TYPE_PZEM
const deviceID = payload.readUInt8(5);
const volt     = payload.readUInt16LE(6) / 16;
const amp      = payload.readUInt16LE(8) / 128;
const cosphi   = payload.readUInt16LE(10) / 128;
const watt     = payload.readUInt16LE(12);
```

---

### `TYPE_TENDE = 0x07` → `struct tendeData` — Tuya blinds

```
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ byte 0   │  byte 1  │  byte 2  │  byte 3  │  byte 4  │  byte 5  │
│ deviceID │  pos[0]  │  pos[1]  │  pos[2]  │  pos[3]  │  pos[4]  │
│ uint8    │  uint8   │  uint8   │  uint8   │  uint8   │  uint8   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
 Payload = 6 byte    Frame totale = 12 byte
```

- `pos[i]` = apertura tenda i-esima, 0–100 %
- `numTende = header.payloadLength - 1` (esclude deviceID)

```javascript
// Node-RED decode TYPE_TENDE
const deviceID = payload.readUInt8(5);
const n        = payload.readUInt16LE(3) - 1;   // payloadLength - 1
const pos      = [];
for (let i = 0; i < n; i++) pos[i] = payload.readUInt8(6 + i);
```

---

## DEV_* → dispositivo (WHO)

| deviceID | Nome          | Sensore  | PacketType  |
|----------|---------------|----------|-------------|
| `0x01`   | Mariner       | BME280   | TYPE_BME    |
| `0x02`   | Salotto       | DHT22    | TYPE_DHT    |
| `0x03`   | Camera        | DHT22    | TYPE_DHT    |
| `0x04`   | Bagno         | DS18B20  | TYPE_DS18   |
| `0x05`   | Tuya Tende    | (bridge) | TYPE_TENDE  |
| `0xFF`   | Unknown       | —        | —           |

> **Aggiungere un nuovo nodo** = un nuovo `DEV_*` qui.  
> **Aggiungere un nuovo tipo di sensore** = un nuovo `TYPE_*` + struct.  
> Le due dimensioni sono indipendenti.

---

## Ricezione — pattern C++ consigliato

```cpp
// Early-return pattern: nessun if annidato
if (payload[0] != PACKET_MAGIC || length < PACKET_MIN_SIZE) return;

ParsedPacket pkt;
if (pp_parsePacket(payload, length, &pkt) != 0) return;

switch (pkt.header.type) {

    case TYPE_DHT: {
        const dhtData *d = (const dhtData *)pkt.payload;
        float t = PP_DECODE_TEMP(d->temperature);
        float h = PP_DECODE_HUM(d->humidity);
        // d->deviceID distingue salotto / camera / ...
        break;
    }
    case TYPE_DS18: {
        const ds18Data *d = (const ds18Data *)pkt.payload;
        float t = PP_DECODE_TEMP(d->temperature);
        break;
    }
    case TYPE_BME: {
        const bmeData *d = (const bmeData *)pkt.payload;
        float t = PP_DECODE_TEMP(d->temperature);
        float h = PP_DECODE_HUM(d->humidity);
        float p = PP_DECODE_PRESS(d->pressure);
        break;
    }
    case TYPE_PZEM: {
        const EneMainData *d = (const EneMainData *)pkt.payload;
        float v = d->v / 16.0f;
        float i = d->i / 128.0f;
        float c = d->c / 128.0f;
        break;
    }
    case TYPE_TENDE: {
        const tendeData *d = (const tendeData *)pkt.payload;
        uint8_t n = pkt.header.payloadLength - 1;
        for (int i = 0; i < n && i < 5; i++) {
            uint8_t pos = d->pos[i];
            // usa pos...
        }
        break;
    }
}
```

---

*File: `PacketProtocol.h` · `PacketProtocol.cpp` · `devices.h`*
