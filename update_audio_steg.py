import re
import os

filepath = "src/steganography.c"
with open(filepath, 'r') as f:
    content = f.read()

new_encode_audio = """void EncodeMessageInAudio(AppState *state, const char* audioPath, const char* message, const char* outputPath) {
    if (!FileExists(audioPath)) {
        ShowStatus(state, "Audio file not found");
        return;
    }
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatus(state, "Failed to load audio");
        return;
    }
    
    WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    
    int originalMessageLen = strlen(message);
    if (originalMessageLen > MAX_MESSAGE_LENGTH) { 
        UnloadWave(wave);
        ShowStatus(state, "Hidden message too long");
        return;
    }
    
    int encodedMessageLen = originalMessageLen;
    unsigned char* encodedData = (unsigned char*)message;
    unsigned char* encryptedData = NULL;

    if (state->useEncryption) {
        encryptedData = Crypto_EncryptAES256((unsigned char*)message, originalMessageLen, (unsigned char*)state->encryptionKey, &encodedMessageLen);
        if (encryptedData) {
            encodedData = encryptedData;
        } else {
            UnloadWave(wave);
            ShowStatus(state, "Encryption failed");
            return;
        }
    }

    // Header: [Magic: 4 bytes "STG1"] [Encrypted: 1 byte] [Length: 4 bytes] [CRC32: 4 bytes]
    int headerLen = 13;
    int totalDataLen = headerLen + encodedMessageLen;
    
    if (wave.frameCount * wave.channels < (totalDataLen) * 8) {
        if (encryptedData) free(encryptedData);
        UnloadWave(wave);
        ShowStatus(state, "Audio too short for message");
        return;
    }
    
    unsigned char header[13];
    header[0] = 'S'; header[1] = 'T'; header[2] = 'G'; header[3] = '1';
    header[4] = state->useEncryption ? 1 : 0;
    header[5] = (encodedMessageLen >> 24) & 0xFF;
    header[6] = (encodedMessageLen >> 16) & 0xFF;
    header[7] = (encodedMessageLen >> 8) & 0xFF;
    header[8] = encodedMessageLen & 0xFF;
    
    uint32_t crc = Crypto_CRC32(encodedData, encodedMessageLen);
    header[9] = (crc >> 24) & 0xFF;
    header[10] = (crc >> 16) & 0xFF;
    header[11] = (crc >> 8) & 0xFF;
    header[12] = crc & 0xFF;

    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    for (int i = 0; i < totalDataLen; i++) {
        unsigned char byteToEmbed = (i < headerLen) ? header[i] : encodedData[i - headerLen];
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = i * 8 + bit;
            if (sampleIndex >= totalSamples) break;
            
            int bitValue = (byteToEmbed >> (7 - bit)) & 1;
            samples[sampleIndex] = (samples[sampleIndex] & 0xFFFE) | bitValue;
        }
    }
    
    bool success = ExportWave(wave, outputPath);
    UnloadWave(wave);
    if (encryptedData) free(encryptedData);
    
    if (!success) {
        ShowStatus(state, "Failed to save encoded audio");
    }
}"""

content = re.sub(r'void EncodeMessageInAudio.*?^}', new_encode_audio, content, flags=re.MULTILINE | re.DOTALL)

new_decode_audio = """char* DecodeMessageFromAudio(AppState *state, const char* audioPath) {
    if (!FileExists(audioPath)) return NULL;
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) return NULL;
    
    WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    
    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    unsigned char header[13];
    int headerLen = 13;
    
    if (totalSamples < headerLen * 8) {
        UnloadWave(wave);
        return NULL;
    }
    
    for (int i = 0; i < headerLen; i++) {
        unsigned char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = i * 8 + bit;
            int bitValue = samples[sampleIndex] & 1;
            ch |= (bitValue << (7 - bit));
        }
        header[i] = ch;
    }
    
    if (header[0] != 'S' || header[1] != 'T' || header[2] != 'G' || header[3] != '1') {
        UnloadWave(wave);
        return NULL;
    }
    
    bool isEncrypted = header[4] == 1;
    int messageLen = (header[5] << 24) | (header[6] << 16) | (header[7] << 8) | header[8];
    uint32_t expectedCrc = (header[9] << 24) | (header[10] << 16) | (header[11] << 8) | header[12];
    
    if (messageLen <= 0 || messageLen > MAX_MESSAGE_LENGTH * 2 || totalSamples < (headerLen + messageLen) * 8) { 
        UnloadWave(wave);
        return NULL;
    }
    
    unsigned char* decodedData = (unsigned char*)malloc(messageLen + 1);
    if (!decodedData) {
        UnloadWave(wave);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        unsigned char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = (headerLen + i) * 8 + bit;
            int bitValue = samples[sampleIndex] & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedData[i] = ch;
    }
    decodedData[messageLen] = '\0';
    
    uint32_t actualCrc = Crypto_CRC32(decodedData, messageLen);
    if (actualCrc != expectedCrc) {
        free(decodedData);
        UnloadWave(wave);
        return NULL;
    }
    
    if (isEncrypted) {
        if (!state->useEncryption) {
            free(decodedData);
            UnloadWave(wave);
            return NULL;
        }
        int outLen;
        unsigned char* decryptedData = Crypto_DecryptAES256(decodedData, messageLen, (unsigned char*)state->encryptionKey, &outLen);
        free(decodedData);
        if (decryptedData) {
            char* str = (char*)malloc(outLen + 1);
            memcpy(str, decryptedData, outLen);
            str[outLen] = '\0';
            free(decryptedData);
            decodedData = (unsigned char*)str;
        } else {
            UnloadWave(wave);
            return NULL;
        }
    }
    
    UnloadWave(wave);
    return (char*)decodedData;
}"""

content = re.sub(r'char\* DecodeMessageFromAudio.*?^}', new_decode_audio, content, flags=re.MULTILINE | re.DOTALL)

with open(filepath, 'w') as f:
    f.write(content)
