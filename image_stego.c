#include "globals.h"
#include "utils.h"
#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating random image...");
    
    int width = 512;
    int height = 512;
    Image image = GenImageColor(width, height, WHITE);

    // Color every pixel with a random color
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

    ExportImage(image, "randomImage.png");
    UnloadImage(image);
    ShowStatusMessage("Random image generated successfully!");
}


void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath) {
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatusMessage("Failed to load image!");
        return;
    }
    
    int messageLen = strlen(message);
    int totalBits = (messageLen + 1) * 8; // +1 for null terminator
    int totalPixels = image.width * image.height;
    
    if (totalBits > totalPixels * 3) {
        ShowStatusMessage("Message too long for this image!");
        UnloadImage(image);
        return;
    }
    
    // Convert to format we can modify
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* pixels = (Color*)image.data;
    
    // Encode message length first (4 bytes)
    int bitIndex = 0;
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 3;
        int colorComponent = i % 3;
        int bit = (messageLen >> (31 - i)) & 1;
        
        Color pixel = pixels[pixelIndex];
        unsigned char* component = NULL;
        
        switch (colorComponent) {
            case 0: component = &pixel.r; break;
            case 1: component = &pixel.g; break;
            case 2: component = &pixel.b; break;
        }
        
        *component = (*component & 0xFE) | bit;
        pixels[pixelIndex] = pixel;
    }
    
    // Encode message
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int pixelIndex = (32 + i * 8 + bit) / 3;
            int colorComponent = (32 + i * 8 + bit) % 3;
            int bitValue = (ch >> (7 - bit)) & 1;
            
            Color pixel = pixels[pixelIndex];
            unsigned char* component = NULL;
            
            switch (colorComponent) {
                case 0: component = &pixel.r; break;
                case 1: component = &pixel.g; break;
                case 2: component = &pixel.b; break;
            }
            
            *component = (*component & 0xFE) | bitValue;
            pixels[pixelIndex] = pixel;
        }
    }
    
    ExportImage(image, outputPath);
    UnloadImage(image);
    ShowStatusMessage("Message encoded successfully!");
}

char* DecodeMessageFromImage(const char* imagePath) {
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatusMessage("Failed to load image!");
        return NULL;
    }
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* pixels = (Color*)image.data;
    
    // Decode message length first
    int messageLen = 0;
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 3;
        int colorComponent = i % 3;
        
        Color pixel = pixels[pixelIndex];
        unsigned char component = 0;
        
        switch (colorComponent) {
            case 0: component = pixel.r; break;
            case 1: component = pixel.g; break;
            case 2: component = pixel.b; break;
        }
        
        int bit = component & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 1000) {
        ShowStatusMessage("Invalid message length or no hidden message!");
        UnloadImage(image);
        return NULL;
    }
    
    // Decode message
    char* decodedMessage = (char*)malloc(messageLen + 1);
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int pixelIndex = (32 + i * 8 + bit) / 3;
            int colorComponent = (32 + i * 8 + bit) % 3;
            
            Color pixel = pixels[pixelIndex];
            unsigned char component = 0;
            
            switch (colorComponent) {
                case 0: component = pixel.r; break;
                case 1: component = pixel.g; break;
                case 2: component = pixel.b; break;
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
