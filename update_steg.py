import re
import os

filepath = "src/steganography.c"
with open(filepath, 'r') as f:
    content = f.read()

# Add crypto.h include and max length
content = content.replace('#include "utils.h"', '#include "utils.h"\n#include "crypto.h"')

# Replace image encoding function to include encryption and larger max size
new_encode_img = """void EncodeMessageInImage(AppState *state, const char* imagePath, const char* message, const char* outputPath) {
    if (!FileExists(imagePath)) {
        ShowStatus(state, "Image file not found");
        return;
    }
    
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatus(state, "Failed to load image");
        return;
    }
    
    int originalMessageLen = strlen(message);
    if (originalMessageLen > MAX_MESSAGE_LENGTH) { 
        UnloadImage(image);
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
            UnloadImage(image);
            ShowStatus(state, "Encryption failed");
            return;
        }
    }

    // Header: [Magic: 4 bytes "STG1"] [Encrypted: 1 byte] [Length: 4 bytes] [CRC32: 4 bytes]
    int headerLen = 13;
    int totalDataLen = headerLen + encodedMessageLen;
    
    if (image.width * image.height < (totalDataLen) * 8) {
        if (encryptedData) free(encryptedData);
        UnloadImage(image);
        ShowStatus(state, "Image too small for message");
        return;
    }
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (image.data == NULL) {
        if (encryptedData) free(encryptedData);
        UnloadImage(image);
        ShowStatus(state, "Failed to format image");
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

    Color* pixels = (Color*)image.data;
    int totalPixels = image.width * image.height;
    
    // Embed header + data
    for (int i = 0; i < totalDataLen; i++) {
        unsigned char byteToEmbed = (i < headerLen) ? header[i] : encodedData[i - headerLen];
        for (int bit = 0; bit < 8; bit++) {
            int idx = i * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            int bitValue = (byteToEmbed >> (7 - bit)) & 1;
            
            unsigned char* component = NULL;
            switch (colorComponent) {
                case 0: component = &pixels[pixelIndex].r; break;
                case 1: component = &pixels[pixelIndex].g; break;
                case 2: component = &pixels[pixelIndex].b; break;
                case 3: component = &pixels[pixelIndex].a; break;
            }
            if (component) *component = (*component & 0xFE) | bitValue;
        }
    }
    
    bool success = ExportImage(image, outputPath);
    UnloadImage(image);
    if (encryptedData) free(encryptedData);
    
    if (!success) {
        ShowStatus(state, "Failed to save encoded image");
    }
}"""

content = re.sub(r'void EncodeMessageInImage.*?^}', new_encode_img, content, flags=re.MULTILINE | re.DOTALL)

new_decode_img = """char* DecodeMessageFromImage(AppState *state, const char* imagePath) {
    if (!FileExists(imagePath)) return NULL;
    
    Image image = LoadImage(imagePath);
    if (image.data == NULL) return NULL;
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (image.data == NULL) {
        UnloadImage(image);
        return NULL;
    }
    
    Color* pixels = (Color*)image.data;
    int totalPixels = image.width * image.height;
    
    unsigned char header[13];
    int headerLen = 13;
    
    for (int i = 0; i < headerLen; i++) {
        unsigned char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int idx = i * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            unsigned char component = 0;
            switch (colorComponent) {
                case 0: component = pixels[pixelIndex].r; break;
                case 1: component = pixels[pixelIndex].g; break;
                case 2: component = pixels[pixelIndex].b; break;
                case 3: component = pixels[pixelIndex].a; break;
            }
            
            int bitValue = component & 1;
            ch |= (bitValue << (7 - bit));
        }
        header[i] = ch;
    }
    
    if (header[0] != 'S' || header[1] != 'T' || header[2] != 'G' || header[3] != '1') {
        UnloadImage(image);
        return NULL; // Legacy format or not encoded
    }
    
    bool isEncrypted = header[4] == 1;
    int messageLen = (header[5] << 24) | (header[6] << 16) | (header[7] << 8) | header[8];
    uint32_t expectedCrc = (header[9] << 24) | (header[10] << 16) | (header[11] << 8) | header[12];
    
    if (messageLen <= 0 || messageLen > MAX_MESSAGE_LENGTH * 2) { 
        UnloadImage(image);
        return NULL;
    }
    
    unsigned char* decodedData = (unsigned char*)malloc(messageLen + 1);
    if (!decodedData) {
        UnloadImage(image);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        unsigned char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int idx = (headerLen + i) * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            unsigned char component = 0;
            switch (colorComponent) {
                case 0: component = pixels[pixelIndex].r; break;
                case 1: component = pixels[pixelIndex].g; break;
                case 2: component = pixels[pixelIndex].b; break;
                case 3: component = pixels[pixelIndex].a; break;
            }
            
            int bitValue = component & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedData[i] = ch;
    }
    decodedData[messageLen] = '\0';
    
    uint32_t actualCrc = Crypto_CRC32(decodedData, messageLen);
    if (actualCrc != expectedCrc) {
        free(decodedData);
        UnloadImage(image);
        return NULL; // CRC failure
    }
    
    if (isEncrypted) {
        if (!state->useEncryption) {
            // Need a key to decode
            free(decodedData);
            UnloadImage(image);
            return NULL;
        }
        int outLen;
        unsigned char* decryptedData = Crypto_DecryptAES256(decodedData, messageLen, (unsigned char*)state->encryptionKey, &outLen);
        free(decodedData);
        if (decryptedData) {
            // Realloc or just return since we null-terminate
            char* str = (char*)malloc(outLen + 1);
            memcpy(str, decryptedData, outLen);
            str[outLen] = '\0';
            free(decryptedData);
            decodedData = (unsigned char*)str;
        } else {
            UnloadImage(image);
            return NULL;
        }
    }
    
    UnloadImage(image);
    return (char*)decodedData;
}"""
content = re.sub(r'char\* DecodeMessageFromImage.*?^}', new_decode_img, content, flags=re.MULTILINE | re.DOTALL)

with open(filepath, 'w') as f:
    f.write(content)
