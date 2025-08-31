#include "globals.h"
#include "utils.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void GenerateRandomAudio() {
    TraceLog(LOG_INFO, "Generating random audio...");
    
    int sampleRate = 44100;
    int duration = 5; // 5 seconds
    int sampleCount = sampleRate * duration;
    
    Wave wave = { 0 };
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* samples = (short*)malloc(sampleCount * sizeof(short));
    
    // Generate a simple sine wave with some noise
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float frequency = 440.0f + sin(t * 2.0f) * 100.0f; // Varying frequency
        float sample = sin(2.0f * PI * frequency * t) * 0.5f;
        sample += (GetRandomValue(-1000, 1000) / 10000.0f) * 0.1f; // Add some noise
        samples[i] = (short)(sample * 32767);
    }
    
    wave.data = samples;
    
    ExportWave(wave, "randomAudio.wav");
    UnloadWave(wave);
    ShowStatusMessage("Random audio generated successfully!");
}

void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath) {
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatusMessage("Failed to load audio file!");
        return;
    }
    
    int messageLen = strlen(message);
    int totalBits = (messageLen + 1) * 8;
    
    if (totalBits > wave.frameCount) {
        ShowStatusMessage("Message too long for this audio file!");
        UnloadWave(wave);
        return;
    }
    
    // Convert to 16-bit if needed
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, 44100, 16, wave.channels);
    }
    
    short* samples = (short*)wave.data;
    
    // Encode message length first (32 bits)
    for (int i = 0; i < 32; i++) {
        int bit = (messageLen >> (31 - i)) & 1;
        if (bit) {
            samples[i] |= 1; // Set LSB
        } else {
            samples[i] &= ~1; // Clear LSB
        }
    }
    
    // Encode message
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            int bitValue = (ch >> (7 - bit)) & 1;
            
            if (bitValue) {
                samples[sampleIndex] |= 1;
            } else {
                samples[sampleIndex] &= ~1;
            }
        }
    }
    
    ExportWave(wave, outputPath);
    UnloadWave(wave);
    ShowStatusMessage("Message encoded in audio successfully!");
}

char* DecodeMessageFromAudio(const char* audioPath) {
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatusMessage("Failed to load audio file!");
        return NULL;
    }
    
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, 44100, 16, wave.channels);
    }
    
    short* samples = (short*)wave.data;
    
    // Decode message length
    int messageLen = 0;
    for (int i = 0; i < 32; i++) {
        int bit = samples[i] & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 1000) {
        ShowStatusMessage("Invalid message length or no hidden message!");
        UnloadWave(wave);
        return NULL;
    }
    
    // Decode message
    char* decodedMessage = (char*)malloc(messageLen + 1);
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            int bitValue = samples[sampleIndex] & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadWave(wave);
    return decodedMessage;
}