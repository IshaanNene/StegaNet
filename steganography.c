#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "steganography.h"
#include "utils.h" 

void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath) {
    if (!FileExists(imagePath)) {
        ShowStatus("Image file not found");
        return;
    }
    
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatus("Failed to load image");
        return;
    }
    
    int messageLen = strlen(message);
    if (messageLen > 100) { 
        UnloadImage(image);
        ShowStatus("Hidden message too long");
        return;
    }
    if (image.width * image.height < (messageLen + 1) * 8 + 32) {
        UnloadImage(image);
        ShowStatus("Image too small for message");
        return;
    }
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (image.data == NULL) {
        UnloadImage(image);
        ShowStatus("Failed to format image");
        return;
    }
    
    Color* pixels = (Color*)image.data;
    int totalPixels = image.width * image.height;
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 4;
        if (pixelIndex >= totalPixels) break;
        
        int colorComponent = i % 4;
        int bit = (messageLen >> (31 - i)) & 1;
        
        unsigned char* component = NULL;
        switch (colorComponent) {
            case 0: component = &pixels[pixelIndex].r; break;
            case 1: component = &pixels[pixelIndex].g; break;
            case 2: component = &pixels[pixelIndex].b; break;
            case 3: component = &pixels[pixelIndex].a; break;
        }
        if (component) *component = (*component & 0xFE) | bit;
    }
    
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int idx = 32 + i * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            int bitValue = (ch >> (7 - bit)) & 1;
            
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
    
    if (!success) {
        ShowStatus("Failed to save encoded image");
    }
}

char* DecodeMessageFromImage(const char* imagePath) {
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
    int messageLen = 0;
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 4;
        if (pixelIndex >= totalPixels) break;
        
        int colorComponent = i % 4;
        
        unsigned char component = 0;
        switch (colorComponent) {
            case 0: component = pixels[pixelIndex].r; break;
            case 1: component = pixels[pixelIndex].g; break;
            case 2: component = pixels[pixelIndex].b; break;
            case 3: component = pixels[pixelIndex].a; break;
        }
        
        int bit = component & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 100) { 
        UnloadImage(image);
        return NULL;
    }
    
    char* decodedMessage = (char*)malloc(messageLen + 1);
    if (!decodedMessage) {
        UnloadImage(image);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int idx = 32 + i * 8 + bit;
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
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadImage(image);
    return decodedMessage;
}

void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath) {
    if (!FileExists(audioPath)) {
        ShowStatus("Audio file not found");
        return;
    }
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatus("Failed to load audio file");
        return;
    }
    
    int messageLen = strlen(message);
    if (messageLen > 100) { 
        UnloadWave(wave);
        ShowStatus("Hidden message too long for audio");
        return;
    }
    
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    if (wave.data == NULL) {
        UnloadWave(wave);
        ShowStatus("Failed to format audio");
        return;
    }
    
    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    if (totalSamples < (messageLen + 1) * 8 + 32) {
        UnloadWave(wave);
        ShowStatus("Audio file too short for message");
        return;
    }
    
    for (int i = 0; i < 32 && i < totalSamples; i++) {
        int bit = (messageLen >> (31 - i)) & 1;
        if (bit) samples[i] |= 1;
        else samples[i] &= ~1;
    }
    
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex >= totalSamples) break;
            
            int bitValue = (ch >> (7 - bit)) & 1;
            if (bitValue) samples[sampleIndex] |= 1;
            else samples[sampleIndex] &= ~1;
        }
    }
    
    bool success = ExportWave(wave, outputPath);
    UnloadWave(wave);
    
    if (!success) {
        ShowStatus("Failed to save encoded audio");
    }
}

char* DecodeMessageFromAudio(const char* audioPath) {
    if (!FileExists(audioPath)) return NULL;
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) return NULL;
    
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    if (wave.data == NULL) {
        UnloadWave(wave);
        return NULL;
    }
    
    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    int messageLen = 0;
    for (int i = 0; i < 32 && i < totalSamples; i++) {
        int bit = samples[i] & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 100) {
        UnloadWave(wave);
        return NULL;
    }
    
    char* decodedMessage = (char*)malloc(messageLen + 1);
    if (!decodedMessage) {
        UnloadWave(wave);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex >= totalSamples) break;
            
            int bitValue = samples[sampleIndex] & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadWave(wave);
    return decodedMessage;
}
