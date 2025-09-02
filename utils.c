#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h> 

#include "common.h"
#include "utils.h"

void AddMessage(const char* sender, const char* content, MessageType type, bool isSent) {
    if (!sender || !content) return;
    
    if (messageCount >= MAX_MESSAGES) {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            messages[i] = messages[i + 1];
        }
        messageCount--;
    }
    
    ChatMessage* msg = &messages[messageCount++];
    strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    msg->sender[sizeof(msg->sender) - 1] = '\0';
    strncpy(msg->content, content, sizeof(msg->content) - 1);
    msg->content[sizeof(msg->content) - 1] = '\0';
    msg->type = type;
    msg->isSent = isSent;
    msg->hasHiddenMessage = false;
    msg->hiddenMessage[0] = '\0';
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info) {
        strftime(msg->timestamp, sizeof(msg->timestamp), "%H:%M", tm_info);
    } else {
        strcpy(msg->timestamp, "??:??");
    }
}

void ShowStatus(const char* message) {
    if (message) {
        strncpy(statusMessage, message, sizeof(statusMessage) - 1);
        statusMessage[sizeof(statusMessage) - 1] = '\0';
        statusTimer = 3.0f;
    }
}

void DownloadFromYoutube(const char* url) {
    if (isDownloading) {
        ShowStatus("Download already in progress...");
        return;
    }
    
    if (strlen(url) == 0) {
        ShowStatus("Please enter a YouTube URL");
        return;
    }
    
    isDownloading = true;
    ShowStatus("Starting download...");
    
    char command[1024];
    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        ShowStatus("Failed to get working directory");
        isDownloading = false;
        return;
    }
    
    snprintf(command, sizeof(command),
         "node \"%s/../yt_downloader.js\" \"%s\"",
         cwd, url);

    FILE* pipe = popen(command, "r");
    if (!pipe) {
        ShowStatus("Failed to start download process");
        isDownloading = false;
        return;
    }
    
    char result[512] = {0};
    char line[256];
    
    while (fgets(line, sizeof(line), pipe) != NULL) {
        strncpy(result, line, sizeof(result) - 1);
        result[sizeof(result) - 1] = '\0';
    }
    
    int exit_status = pclose(pipe);
    char* newline = strchr(result, '\n');
    if (newline) *newline = '\0';
    
    if (exit_status == 0 && strlen(result) > 0 && 
        strstr(result, "Error:") == NULL && strstr(result, "ERROR:") == NULL) {
        strncpy(selectedFilePath, result, sizeof(selectedFilePath) - 1);
        selectedFilePath[sizeof(selectedFilePath) - 1] = '\0';
        selectedMessageType = MSG_AUDIO;
        ShowStatus("Download complete! Audio ready to encode/send.");
    } else if (strlen(result) > 0) {
        ShowStatus(result);
    } else {
        ShowStatus("Download failed - check URL and internet connection");
    }
    
    isDownloading = false;
}

void GenerateRandomAudio() {
    TraceLog(LOG_INFO, "Generating random audio...");
    
    int sampleRate = 44100;
    int duration = 5; 
    int sampleCount = sampleRate * duration;
    
    Wave wave = { 0 };
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* samples = (short*)malloc(sampleCount * sizeof(short));
    if (!samples) {
        ShowStatus("Memory allocation failed for audio generation");
        return;
    }
    
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float frequency = 440.0f + sin(t * 2.0f) * 100.0f; 
        float sample = sin(2.0f * PI * frequency * t) * 0.5f;
        sample += (GetRandomValue(-1000, 1000) / 10000.0f) * 0.1f;
        samples[i] = (short)(sample * 32767);
    }
    
    wave.data = samples;
    
    bool success = ExportWave(wave, "randomAudio.wav");
    UnloadWave(wave);
    
    if (success) {
        ShowStatus("Random audio generated: randomAudio.wav");
        strcpy(selectedFilePath, "randomAudio.wav");
        selectedMessageType = MSG_AUDIO;
    } else {
        ShowStatus("Failed to generate random audio");
    }
}

void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating random image...");
    
    int width = 512;
    int height = 512;
    Image image = GenImageColor(width, height, WHITE);
    
    if (image.data == NULL) {
        ShowStatus("Failed to create image");
        return;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color color = {
                GetRandomValue(0, 255),
                GetRandomValue(0, 255),
                GetRandomValue(0, 255),
                255
            };
            ImageDrawPixel(&image, x, y, color);
        }
    }

    bool success = ExportImage(image, "randomImage.png");
    UnloadImage(image);
    
    if (success) {
        ShowStatus("Random image generated: randomImage.png");
        strcpy(selectedFilePath, "randomImage.png");
        selectedMessageType = MSG_IMAGE;
    } else {
        ShowStatus("Failed to generate random image");
    }
}
void LoadImageForDisplay(const char* imagePath) {
    if (imageLoaded) {
        UnloadTexture(currentImageTexture);
        imageLoaded = false;
    }
    
    if (FileExists(imagePath)) {
        Image image = LoadImage(imagePath);
        if (image.data != NULL) {
            if (image.width > 400 || image.height > 300) {
                float scale = fminf(400.0f / image.width, 300.0f / image.height);
                int newWidth = (int)(image.width * scale);
                int newHeight = (int)(image.height * scale);
                ImageResize(&image, newWidth, newHeight);
            }
            currentImageTexture = LoadTextureFromImage(image);
            UnloadImage(image);
            imageLoaded = true;
        }

    if (imageLoaded) {
        UnloadTexture(currentImageTexture);
    }
    
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (messageImageLoaded[i]) {
            UnloadTexture(messageImages[i]);
        }
        if (messageAudioLoaded[i]) {
            UnloadSound(messageAudioSounds[i]);
        }
    }
}
}
