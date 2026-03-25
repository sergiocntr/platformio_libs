const mqtt = require('mqtt');

// --- Configurazione Topics ---
const meteotopic = 'homie/esterno/meteo/data';
const meteoacktopic = 'homie/esterno/meteo/ack';
const meteochronotopic = 'homie/esterno/sensori';

// --- Configurazione Broker MQTT ---
const brokerUrl = 'mqtt://192.168.1.100:1883';
const options = {
    clientId: 'nodeDebian',
    username: 'sergio',
    password: 'donatella',
    clean: true,
};

// --- Costanti Protocollo (New Standard) ---
const PACKET_MAGIC = 0xAA;
const PACKET_VERSION = 0x01;
const TYPE_METEO = 0x03;
const HEADER_SIZE = 5;
const PACKET_OVERHEAD = 6; // 5 Header + 1 Checksum

// --- Variabili Globali Ricezione ---
let receivedStructs = [];  
let totalStructs = 0;      
let structIndex = 0;       
let errorCount = 0; 
const maxRetries = 2; 
const numStructsInMessage = 4;
const structSize = 16;

// --- Funzione di debug per dump esadecimale ---
function hexDump(buffer, length = 32) {
    const bytes = Math.min(buffer.length, length);
    let dump = '';
    for (let i = 0; i < bytes; i++) {
        dump += buffer[i].toString(16).padStart(2, '0').toUpperCase() + ' ';
        if ((i + 1) % 8 === 0) dump += ' ';
        if ((i + 1) % 16 === 0) dump += '\n';
    }
    if (buffer.length > length) dump += `... (${buffer.length - length} bytes in piu)`;
    return dump.trim();
}

/**
 * Connessione e gestione messaggi
 */
function connectAndSubscrive() {
    const client = mqtt.connect(brokerUrl, options);

    client.on('connect', () => {
        console.log('[INFO] Connesso al broker MQTT con successo!');
        client.subscribe(meteotopic, (err) => {
            if (err) {
                console.error('[ERR] Errore nella sottoscrizione:', err);
            } else {
                console.log(`[INFO] Sottoscritto a ${meteotopic}`);
            }
        });
    });

    client.on('message', (topic, message) => {
        if (topic === meteotopic) {
            handleProtocolPacket(message, client);
        }
    });

    client.on('error', (err) => {
        console.error('[ERR] Errore di connessione MQTT:', err);
    });
}

/**
 * Gestisce il wrapper del protocollo (Header + CRC)
 */
function handleProtocolPacket(message, client) {
    if (!Buffer.isBuffer(message)) {
        console.error('[ERR] Messaggio non è un buffer');
        return;
    }

    console.log('[DEBUG] Pacchetto ricevuto:', message.length, 'bytes');
    console.log('[DEBUG] Hex dump:\n' + hexDump(message));

    // 1. Validazione Header (Magic e Version)
    if (message.length < PACKET_OVERHEAD) {
        console.error(`[ERR] Pacchetto troppo corto: ${message.length} byte (minimo richiesto: ${PACKET_OVERHEAD})`);
        return;
    }

    if (message[0] !== PACKET_MAGIC) {
        console.error(`[ERR] Magic byte errato: atteso 0x${PACKET_MAGIC.toString(16)}, ricevuto 0x${message[0].toString(16)}`);
        return;
    }
    
    if (message[1] !== PACKET_VERSION) {
        console.error(`[ERR] Version errata: attesa 0x${PACKET_VERSION.toString(16)}, ricevuta 0x${message[1].toString(16)}`);
        return;
    }

    const payloadLen = message.readUInt16LE(3);
    const expectedLen = payloadLen + PACKET_OVERHEAD;
    
    if (message.length !== expectedLen) {
        console.error(`[ERR] Lunghezza non corrispondente: Ricevuti=${message.length}, Dichiarati=${expectedLen}`);
        console.error(`[DEBUG] payloadLen dichiarata: ${payloadLen} (0x${payloadLen.toString(16)})`);
        console.error(`[DEBUG] Bytes 3-4: 0x${message[3].toString(16)} 0x${message[4].toString(16)}`);
        return;
    }

    // 2. Validazione Checksum XOR del pacchetto
    let packetXor = 0;
    for (let i = 0; i < message.length - 1; i++) {
        packetXor ^= message[i];
    }

    const receivedChecksum = message[message.length - 1];
    
    if (receivedChecksum !== packetXor) {
        console.error('[ERR] Checksum Pacchetto fallito (XOR mismatch)');
        console.error(`[DEBUG] XOR calcolato: 0x${packetXor.toString(16).padStart(2, '0')}`);
        console.error(`[DEBUG] XOR ricevuto: 0x${receivedChecksum.toString(16).padStart(2, '0')}`);
        console.error('[DEBUG] Primi 16 bytes del pacchetto:', 
            Array.from(message.slice(0, 16)).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' '));
        return;
    }

    // 3. Estrazione Payload ed Elaborazione
    const payload = message.slice(HEADER_SIZE, message.length - 1);
    console.log('[DEBUG] Payload estratto:', payload.length, 'bytes');
    processPayload(payload, client);
}

/**
 * Elabora il payload interno (MeteoData single o multi)
 */
function processPayload(payload, client) {
    const pSize = payload.length;

    // Caso 1: Sync Buffer Circolare (6 byte)
    if (pSize === 6) {
        console.log('[INFO] Ricevuto Sync Buffer Circolare (ignorato nel decoder meteo)');
        return;
    }

    // Caso 2: Record Singolo (16 byte)
    if (pSize === structSize) {
        console.log('[DEBUG] Elaborazione record singolo di 16 bytes');
        if (!verifyStructXOR(payload)) {
            console.error('[ERR] Errore Checksum Interno Struct (Record Singolo)');
            
            // Dettaglio completo della struct fallita
            console.error('[DEBUG] Dump struct fallita (16 bytes):');
            console.error(hexDump(payload));
            
            // Calcola e mostra lo XOR parziale per ogni byte
            let xor = 0;
            console.error('[DEBUG] Calcolo XOR progressivo:');
            for (let i = 0; i < structSize - 1; i++) {
                xor ^= payload[i];
                console.error(`  Byte ${i}: 0x${payload[i].toString(16).padStart(2, '0')} -> XOR parziale: 0x${xor.toString(16).padStart(2, '0')}`);
            }
            console.error(`  XOR finale calcolato: 0x${xor.toString(16).padStart(2, '0')}`);
            console.error(`  Checksum nella struct: 0x${payload[15].toString(16).padStart(2, '0')}`);
            
            // Tentativo di decodifica parziale
            try {
                const partialDecode = {
                    deviceId: payload[0],
                    humidityBMP_raw: payload.readInt16LE(1),
                    temperatureBMP_raw: payload.readInt16LE(3),
                    externalPressure_raw: payload.readInt16LE(5),
                    battery_raw: payload.readUInt16LE(7),
                    counter: payload[14]
                };
                console.error('[DEBUG] Decodifica parziale (valori raw):', partialDecode);
            } catch (e) {
                console.error('[DEBUG] Impossibile decodificare parzialmente la struct');
            }
            
            sendAckMessage(client, 3); // failed
            return;
        }

        const struct = decodeStruct(payload, 0);
        console.log('[INFO] Ricevuto Record Singolo valido:', struct);
        
        receivedStructs.push(struct);
        sendAckMessage(client, 2); // end
        
        // Announce Chrono
        let jsonDataChrono = `{"topic": "Terrazza","Hum": ${struct.humidityBMP},"Temp": ${struct.temperatureBMP},"Batt": ${struct.battery}}`;
        client.publish(meteochronotopic, jsonDataChrono);

        sendToCloud(receivedStructs);
    } 
    // Caso 3: Multi MSG (N record + 2 byte rem)
    else if (pSize === (numStructsInMessage * structSize) + 2) {
        console.log('[DEBUG] Elaborazione messaggio multiplo:', pSize, 'bytes');
        decodeMultiMSG(payload, client);
    } else {
        console.warn(`[WARN] Payload con lunghezza non standard: ${pSize} byte`);
        console.warn('[DEBUG] Dump payload completo:\n' + hexDump(payload));
    }
}

/**
 * Calcola e verifica lo XOR interno della struct con debug
 */
function verifyStructXOR(bufferSlice) {
    let xor = 0;
    const xorProgress = [];
    
    for (let i = 0; i < structSize - 1; i++) {
        xor ^= bufferSlice[i];
        xorProgress.push({byte: i, value: bufferSlice[i], partialXor: xor});
    }
    
    const isValid = xor === bufferSlice[structSize - 1];
    
    // if (!isValid) {
    //     console.log('[DEBUG] verifyStructXOR - Dettaglio calcolo:');
    //     xorProgress.forEach(p => {
    //         console.log(`  Byte ${p.byte.toString().padStart(2)}: 0x${p.value.toString(16).padStart(2, '0')} -> XOR: 0x${p.partialXor.toString(16).padStart(2, '0')}`);
    //     });
    //     console.log(`  Checksum atteso: 0x${xor.toString(16).padStart(2, '0')}`);
    //     console.log(`  Checksum trovato: 0x${bufferSlice[structSize - 1].toString(16).padStart(2, '0')}`);
    // }
    
    return isValid;
}

/**
 * Decodifica i messaggi multipli (storico EEPROM)
 */
function decodeMultiMSG(payload, client) {
    const pSize = payload.length;
    
    // Inizializza sessione se necessario
    if (receivedStructs.length === 0) {
        totalStructs = payload.readUInt16LE(pSize - 2);
        structIndex = 0;
        console.log(`[INFO] Inizio ricezione blocco storico: ${totalStructs} record attesi.`);
        console.log(`[DEBUG] totalStructs value: ${totalStructs} (bytes: 0x${payload[pSize-2].toString(16)} 0x${payload[pSize-1].toString(16)})`);
    }

    let success = true;
    let errorDetails = [];
    
    for (let i = 0; i < numStructsInMessage; i++) {
        const offset = i * structSize;
        if (offset + structSize > pSize - 2) break;

        const structSlice = payload.slice(offset, offset + structSize);
        
        if (verifyStructXOR(structSlice)) {
            const struct = decodeStruct(payload, offset);
            receivedStructs.push(struct);
            structIndex++;
            console.log(`[INFO] Record ${structIndex}/${totalStructs} OK.`);
        } else {
            console.error(`[ERR] Errore Checksum nel record ${structIndex + 1} del blocco.`);
            console.error(`[DEBUG] Dump record fallito (offset ${offset}):`);
            console.error(hexDump(structSlice));
            
            // Salva dettagli per il report finale
            errorDetails.push({
                recordIndex: structIndex + 1,
                offset: offset,
                data: Array.from(structSlice).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' ')
            });
            
            success = false;
        }

        if (structIndex === totalStructs) break;
    }
    
    if (errorDetails.length > 0) {
        console.error('[DEBUG] Riepilogo errori nel blocco:', errorDetails);
    }

    handleMessageResult(success, client);
}

/**
 * Gestione degli ACK per blocchi multipli
 */
function handleMessageResult(success, client) {
    if (success) {
        errorCount = 0;
        if (structIndex === totalStructs) {
            console.log('[INFO] Tutti i record storici ricevuti correttamente.');
            console.log(`[INFO] Totale record: ${receivedStructs.length}`);
            sendAckMessage(client, 2); // end
            receivedStructs.reverse(); // Ordina dal più recente
            sendToCloud(receivedStructs);
            resetReception();
        } else {
            console.log(`[INFO] Avanzamento: ${structIndex}/${totalStructs} record ricevuti`);
            sendAckMessage(client, 1); // ack
        }
    } else {
        errorCount++;
        console.error(`[ERR] Errore nella ricezione del blocco. Tentativo ${errorCount}/${maxRetries}`);
        
        if (errorCount >= 4) {
            console.error('[ERR] Troppi errori consecutivi. Reset sessione.');
            console.error(`[DEBUG] Statistiche finali: ricevuti ${receivedStructs.length}/${totalStructs} record`);
            sendAckMessage(client, 0); // error
            resetReception();
        } else {
            console.warn('[WARN] Richiesta di ritrasmissione blocco.');
            sendAckMessage(client, 3); // failed (richiede ritrasmissione)
        }
    }
}

function resetReception() {
    receivedStructs = [];
    totalStructs = 0;
    structIndex = 0;
    errorCount = 0;
}

/**
 * Decodifica la struct MeteoData (Little Endian)
 */
function decodeStruct(buffer, offset) {
    let rawStruct = {
        deviceId: buffer.readUInt8(offset),
        humidityBMP: buffer.readInt16LE(offset + 1),
        temperatureBMP: buffer.readInt16LE(offset + 3),
        externalPressure: buffer.readInt16LE(offset + 5),
        battery: buffer.readUInt16LE(offset + 7),
        counter: buffer.readUInt8(offset + 14),
        checksum: buffer.readUInt8(offset + 15)
    };

    // Log raw values per debug
    console.log('[DEBUG] Valori raw struct:', rawStruct);

    // Conversione unità (stessi coefficienti del firmware)
    return {
        ...rawStruct,
        humidityBMP: rawStruct.humidityBMP / 128,
        temperatureBMP: (rawStruct.temperatureBMP / 128) - 50,
        externalPressure: rawStruct.externalPressure / 32
    };
}

/**
 * Invia ACK via MQTT
 */
function sendAckMessage(client, code) {
    const ackMessages = { 1: 'ack', 2: 'end', 3: 'failed', 0: 'error' };
    const message = ackMessages[code];
    if (message) {
        client.publish(meteoacktopic, message);
        console.log(`[INFO] Inviato ACK: ${message} (codice ${code})`);
    }
}

/**
 * Invio dati al cloud Altervista
 */
function sendToCloud(data) {
    const jsonData = JSON.stringify(data);
    console.log('[DEBUG] Invio al cloud:', data.length, 'record');
    
    fetch('https://developteamgold.altervista.org/meteofeletto/swpi_logger_JSON.php', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: jsonData
    }).then(response => {
        if (response.ok) {
            console.log('[INFO] Dati sincronizzati con il cloud.');
        } else {
            console.error('[ERR] Errore risposta Cloud:', response.status, response.statusText);
        }
    }).catch(err => {
        console.error('[ERR] Errore connessione Cloud:', err);
    });
}

// Avvio
connectAndSubscrive();
