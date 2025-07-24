#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef enum {
    STATE_MAIN_MENU,
    STATE_IMAGE_STEGO,
    STATE_AUDIO_STEGO,
    STATE_IMAGE_OPTIONS,
    STATE_AUDIO_OPTIONS,
    STATE_IMAGE_ENCODE,
    STATE_IMAGE_DECODE,
    STATE_AUDIO_ENCODE,
    STATE_AUDIO_DECODE
} AppState;


typedef enum {
    IMAGE_UPLOAD,
    IMAGE_GENERATE,
    IMAGE_NONE
} ImageSource;


typedef enum {
    AUDIO_UPLOAD,
    AUDIO_GENERATE,
    AUDIO_YOUTUBE,
    AUDIO_NONE
} AudioSource;


AppState currentState = STATE_MAIN_MENU;
ImageSource selectedImageSource = IMAGE_NONE;
AudioSource selectedAudioSource = AUDIO_NONE;
char ytLinkBuffer[256] = "";
char messageBuffer[512] = "";
char decodedMessageBuffer[512] = "";
char filePathBuffer[256] = "";
bool ytLinkEditMode = false;
bool messageEditMode = false;
bool filePathEditMode = false;
Texture2D encodedTexture;
bool encodedTextureLoaded = false;
Wave audioWave;
bool audioWaveLoaded = false;
Sound audioSound;
bool audioSoundLoaded = false;
bool isPlayingAudio = false;
char statusMessage[256] = "";
float statusMessageTimer = 0;
Color themeColor = {45, 55, 72, 255};
Color accentColor = {72, 187, 120, 255};
Color errorColor = {245, 101, 101, 255};
Color successColor = {56, 178, 172, 255};


void ShowStatusMessage(const char* message) {
    strncpy(statusMessage, message, sizeof(statusMessage) - 1);
    statusMessage[sizeof(statusMessage) - 1] = '\0';
    statusMessageTimer = 3.0f; 
}

void UpdateStatusMessage() {
    if (statusMessageTimer > 0) {
        statusMessageTimer -= GetFrameTime();
        if (statusMessageTimer <= 0) {
            statusMessage[0] = '\0';
        }
    }
}

void DrawStatusMessage() {
    if (statusMessage[0] != '\0') {
        int screenWidth = GetScreenWidth();
        int textWidth = MeasureText(statusMessage, 16);
        
        
        DrawRectangle(screenWidth / 2 - textWidth / 2 - 10, 25, textWidth + 20, 30, Fade(WHITE, 0.9f));
        DrawRectangleLines(screenWidth / 2 - textWidth / 2 - 10, 25, textWidth + 20, 30, accentColor);
        DrawText(statusMessage, screenWidth / 2 - textWidth / 2, 33, 16, themeColor);
    }
}


void DrawGradientButton(Rectangle bounds, const char* text, Color color1, Color color2, bool pressed) {
    if (pressed) {
        DrawRectangleGradientV((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height, color2, color1);
    } else {
        DrawRectangleGradientV((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height, color1, color2);
    }
    DrawRectangleLines((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height, Fade(BLACK, 0.3f));
    
    int textWidth = MeasureText(text, 18);
    DrawText(text, (int)(bounds.x + bounds.width/2 - textWidth/2), 
             (int)(bounds.y + bounds.height/2 - 9), 18, WHITE);
}

void DrawBackgroundPattern(int screenWidth, int screenHeight) {
    
    for (int x = 0; x < screenWidth; x += 60) {
        for (int y = 0; y < screenHeight; y += 60) {
            DrawCircle(x, y, 2, Fade(themeColor, 0.1f));
        }
    }
}


void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating enhanced random image...");
    
    int width = 512;
    int height = 512;
    Image image = GenImageColor(width, height, BLACK);
    
    
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            float fx = (float)x / width;
            float fy = (float)y / height;
            
            
            float wave1 = sin(fx * 20 + fy * 15) * 0.5f + 0.5f;
            float wave2 = cos(fx * 15 + fy * 20) * 0.5f + 0.5f;
            float wave3 = sin((fx + fy) * 25) * 0.5f + 0.5f;
            
            
            float intensity = (wave1 + wave2 + wave3) / 3.0f;
            
            
            intensity += (GetRandomValue(0, 100) / 500.0f);
            
            
            Color color = {
                (unsigned char)(intensity * 255 * (fx + 0.2f)),
                (unsigned char)(intensity * 255 * (fy + 0.2f)),
                (unsigned char)(intensity * 255 * ((1.0f - fx) + (1.0f - fy)) * 0.5f),
                255
            };
            ImageDrawPixel(&image, x, y, color);
        }
    }
    
    
    for (int i = 0; i < 50; i++) {
        int x = GetRandomValue(0, width - 50);
        int y = GetRandomValue(0, height - 50);
        int radius = GetRandomValue(5, 25);
        Color circleColor = {
            GetRandomValue(100, 255),
            GetRandomValue(100, 255),
            GetRandomValue(100, 255),
            GetRandomValue(50, 150)
        };
        ImageDrawCircle(&image, x, y, radius, circleColor);
    }
    
    ExportImage(image, "randomImage.png");
    UnloadImage(image);
    ShowStatusMessage("Enhanced random image generated successfully!");
}


void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath) {
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatusMessage("Failed to load image!");
        return;
    }
    
    int messageLen = strlen(message);
    int totalBits = (messageLen + 1) * 8; 
    int totalPixels = image.width * image.height;
    
    if (totalBits > totalPixels * 3) {
        ShowStatusMessage("Message too long for this image!");
        UnloadImage(image);
        return;
    }
    
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* pixels = (Color*)image.data;
    
    
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
    
    
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int bitPos = 32 + i * 8 + bit;
            int pixelIndex = bitPos / 3;
            int colorComponent = bitPos % 3;
            int bitValue = (ch >> (7 - bit)) & 1;
            
            if (pixelIndex < totalPixels) {
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
    
    if (messageLen <= 0 || messageLen > 10000) {
        ShowStatusMessage("Invalid message length or no hidden message found!");
        UnloadImage(image);
        return NULL;
    }
    
    
    char* decodedMessage = (char*)malloc(messageLen + 1);
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int bitPos = 32 + i * 8 + bit;
            int pixelIndex = bitPos / 3;
            int colorComponent = bitPos % 3;
            
            if (pixelIndex < image.width * image.height) {
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
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadImage(image);
    return decodedMessage;
}


void GenerateRandomAudio() {
    TraceLog(LOG_INFO, "Generating enhanced random audio...");
    
    int sampleRate = 44100;
    int duration = 10; 
    int sampleCount = sampleRate * duration;
    
    Wave wave = { 0 };
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* samples = (short*)malloc(sampleCount * sizeof(short));
    
    
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        
        
        float freq1 = 220.0f + sin(t * 0.5f) * 50.0f;  
        float freq2 = 440.0f + cos(t * 0.7f) * 100.0f; 
        float freq3 = 880.0f + sin(t * 1.2f) * 200.0f; 
        
        
        float sample = 0;
        sample += sin(2.0f * PI * freq1 * t) * 0.3f;
        sample += sin(2.0f * PI * freq2 * t) * 0.2f;
        sample += sin(2.0f * PI * freq3 * t) * 0.1f;
        
        
        float envelope = exp(-t * 0.1f) * (1.0f + sin(t * 2.0f) * 0.3f);
        sample *= envelope;
        
        
        sample += (GetRandomValue(-100, 100) / 50000.0f);
        
        
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        samples[i] = (short)(sample * 32767);
    }
    
    wave.data = samples;
    
    ExportWave(wave, "randomAudio.wav");
    UnloadWave(wave);
    ShowStatusMessage("Enhanced random audio generated successfully!");
}


void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath) {
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatusMessage("Failed to load audio file!");
        return;
    }
    
    int messageLen = strlen(message);
    int totalBits = (messageLen + 1) * 8; 
    
    if (totalBits + 32 > wave.frameCount) { 
        ShowStatusMessage("Message too long for this audio file!");
        UnloadWave(wave);
        return;
    }
    
    
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    short* samples = (short*)wave.data;
    
    
    for (int i = 0; i < 32; i++) {
        int bit = (messageLen >> (31 - i)) & 1;
        if (bit) {
            samples[i] |= 1; 
        } else {
            samples[i] &= ~1; 
        }
    }
    
    
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex < wave.frameCount) {
                int bitValue = (ch >> (7 - bit)) & 1;
                
                if (bitValue) {
                    samples[sampleIndex] |= 1;
                } else {
                    samples[sampleIndex] &= ~1;
                }
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
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    short* samples = (short*)wave.data;
    
    
    int messageLen = 0;
    for (int i = 0; i < 32; i++) {
        int bit = samples[i] & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 10000) {
        ShowStatusMessage("Invalid message length or no hidden message found!");
        UnloadWave(wave);
        return NULL;
    }
    
    
    char* decodedMessage = (char*)malloc(messageLen + 1);
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex < wave.frameCount) {
                int bitValue = samples[sampleIndex] & 1;
                ch |= (bitValue << (7 - bit));
            }
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadWave(wave);
    return decodedMessage;
}

void DownloadFromYouTube(const char* url) {
    char command[512];
    snprintf(command, sizeof(command), "yt-dlp -f 'ba[ext=wav]/ba[ext=m4a]/ba' --extract-audio --audio-format wav --output 'youtube_audio.%%(ext)s' \"%s\"", url);
    int result = system(command);
    if (result == 0) {
        ShowStatusMessage("Audio downloaded successfully!");
    } else {
        ShowStatusMessage("Failed to download audio. Make sure yt-dlp is installed.");
    }
}


void DrawImageEncode() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){240, 248, 255, 255}, (Color){225, 245, 254, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🖼️ Encode Message in Image";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 28, 24, WHITE);

    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        if (encodedTextureLoaded) {
            UnloadTexture(encodedTexture);
            encodedTextureLoaded = false;
        }
        currentState = STATE_IMAGE_OPTIONS;
        return;
    }

    
    Rectangle sourcePanel = {50, 100, screenWidth - 100, 60};
    DrawRectangleRounded(sourcePanel, 0.1f, 10, Fade(WHITE, 0.9f));
    DrawRectangleRoundedLines(sourcePanel, 0.1f, 10, accentColor);
    
    char sourceText[100];
    switch (selectedImageSource) {
        case IMAGE_UPLOAD: strcpy(sourceText, "📂 Source: Uploaded Image"); break;
        case IMAGE_GENERATE: strcpy(sourceText, "🎨 Source: Generated Image"); break;
        default: strcpy(sourceText, "❌ Source: None Selected"); break;
    }
    DrawText(sourceText, (int)sourcePanel.x + 20, (int)sourcePanel.y + 20, 18, themeColor);

    int yPos = 180;

    
    if (selectedImageSource == IMAGE_UPLOAD) {
        DrawText("📁 Image file path:", 50, yPos, 16, themeColor);
        Rectangle filePathRect = {50, yPos + 25, screenWidth - 100, 35};
        DrawRectangleRounded(filePathRect, 0.1f, 10, WHITE);
        DrawRectangleRoundedLines(filePathRect, 0.1f, 10, filePathEditMode ? accentColor : GRAY);
        
        if (GuiTextBox(filePathRect, filePathBuffer, 256, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
        yPos += 75;
    }

    
    DrawText("💬 Enter message to hide:", 50, yPos, 16, themeColor);
    Rectangle messageRect = {50, yPos + 25, screenWidth - 100, 35};
    DrawRectangleRounded(messageRect, 0.1f, 10, WHITE);
    DrawRectangleRoundedLines(messageRect, 0.1f, 10, messageEditMode ? accentColor : GRAY);
    
    if (GuiTextBox(messageRect, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }
    yPos += 75;

    
    const char* imagePath = (selectedImageSource == IMAGE_UPLOAD) ? filePathBuffer : "randomImage.png";
    
    if (!encodedTextureLoaded && FileExists(imagePath)) {
        encodedTexture = LoadTexture(imagePath);
        encodedTextureLoaded = true;
    }

    if (encodedTextureLoaded) {
        
        Rectangle imagePanel = {50, yPos, screenWidth - 100, 250};
        DrawRectangleRounded(imagePanel, 0.1f, 10, Fade(WHITE, 0.9f));
        DrawRectangleRoundedLines(imagePanel, 0.1f, 10, accentColor);
        
        DrawText("🖼️ Image Preview:", (int)imagePanel.x + 20, (int)imagePanel.y + 10, 16, themeColor);
        
        float scale = fmin(200.0f / encodedTexture.width, 200.0f / encodedTexture.height);
        Vector2 imagePos = {
            imagePanel.x + imagePanel.width/2 - (encodedTexture.width * scale)/2,
            imagePanel.y + 40
        };
        DrawTextureEx(encodedTexture, imagePos, 0, scale, WHITE);

        
        Rectangle encodeBtn = {screenWidth/2 - 160, yPos + 260, 140, 45};
        Rectangle saveBtn = {screenWidth/2 + 20, yPos + 260, 140, 45};
        
        bool encodePressed = CheckCollisionPointRec(GetMousePosition(), encodeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool savePressed = CheckCollisionPointRec(GetMousePosition(), saveBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        
        DrawGradientButton(encodeBtn, "🔒 Encode", successColor, Fade(successColor, 0.8f), encodePressed);
        DrawGradientButton(saveBtn, "💾 Save As...", accentColor, Fade(accentColor, 0.8f), savePressed);
        
        if (encodePressed) {
            if (strlen(messageBuffer) > 0) {
                EncodeMessageInImage(imagePath, messageBuffer, "encoded_image.png");
            } else {
                ShowStatusMessage("⚠️ Please enter a message to encode!");
            }
        }
        
        if (savePressed) {
            ShowStatusMessage("✅ Encoded image saved as 'encoded_image.png'");
        }
    } else if (selectedImageSource == IMAGE_UPLOAD && strlen(filePathBuffer) > 0) {
        Rectangle errorPanel = {50, yPos, screenWidth - 100, 60};
        DrawRectangleRounded(errorPanel, 0.1f, 10, Fade(errorColor, 0.2f));
        DrawRectangleRoundedLines(errorPanel, 0.1f, 10, errorColor);
        DrawText("❌ Image not found! Please check the file path.", (int)errorPanel.x + 20, (int)errorPanel.y + 20, 16, errorColor);
    }

    DrawStatusMessage();
}

void DrawImageDecode() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){240, 248, 255, 255}, (Color){225, 245, 254, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🔓 Decode Message from Image";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 28, 24, WHITE);

    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_IMAGE_STEGO;
        return;
    }

    
    DrawText("📁 Image file path:", 50, 120, 16, themeColor);
    Rectangle filePathRect = {50, 145, screenWidth - 100, 35};
    DrawRectangleRounded(filePathRect, 0.1f, 10, WHITE);
    DrawRectangleRoundedLines(filePathRect, 0.1f, 10, filePathEditMode ? accentColor : GRAY);
    
    if (GuiTextBox(filePathRect, filePathBuffer, 256, filePathEditMode)) {
        filePathEditMode = !filePathEditMode;
    }

    
    Rectangle decodeBtn = {screenWidth/2 - 70, 200, 140, 45};
    bool decodePressed = CheckCollisionPointRec(GetMousePosition(), decodeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(decodeBtn, "🔍 Decode", successColor, Fade(successColor, 0.8f), decodePressed);
    
    if (decodePressed) {
        if (strlen(filePathBuffer) > 0) {
            char* decoded = DecodeMessageFromImage(filePathBuffer);
            if (decoded) {
                strncpy(decodedMessageBuffer, decoded, sizeof(decodedMessageBuffer) - 1);
                decodedMessageBuffer[sizeof(decodedMessageBuffer) - 1] = '\0';
                free(decoded);
                ShowStatusMessage("✅ Message decoded successfully!");
            }
        } else {
            ShowStatusMessage("⚠️ Please enter a file path!");
        }
    }

    
    if (strlen(decodedMessageBuffer) > 0) {
        Rectangle messagePanel = {50, 270, screenWidth - 100, 150};
        DrawRectangleRounded(messagePanel, 0.1f, 10, Fade(WHITE, 0.9f));
        DrawRectangleRoundedLines(messagePanel, 0.1f, 10, successColor);
        
        DrawText("📝 Decoded Message:", (int)messagePanel.x + 20, (int)messagePanel.y + 15, 16, themeColor);
        
        
        int maxWidth = (int)messagePanel.width - 40;
        int lineHeight = 20;
        int currentY = (int)messagePanel.y + 45;
        
        char* words = strdup(decodedMessageBuffer);
        char* word = strtok(words, " ");
        char currentLine[256] = "";
        
        while (word != NULL) {
            char tempLine[256];
            snprintf(tempLine, sizeof(tempLine), "%s%s%s", currentLine, strlen(currentLine) > 0 ? " " : "", word);
            
            if (MeasureText(tempLine, 16) <= maxWidth) {
                strcpy(currentLine, tempLine);
            } else {
                if (strlen(currentLine) > 0) {
                    DrawText(currentLine, (int)messagePanel.x + 20, currentY, 16, themeColor);
                    currentY += lineHeight;
                }
                strcpy(currentLine, word);
            }
            word = strtok(NULL, " ");
        }
        
        if (strlen(currentLine) > 0) {
            DrawText(currentLine, (int)messagePanel.x + 20, currentY, 16, themeColor);
        }
        
        free(words);
    }

    DrawStatusMessage();
}

void DrawAudioEncode() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){248, 250, 252, 255}, (Color){237, 242, 247, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🎵 Encode Message in Audio";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 28, 24, WHITE);

    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        if (audioWaveLoaded) {
            UnloadWave(audioWave);
            audioWaveLoaded = false;
        }
        if (audioSoundLoaded) {
            UnloadSound(audioSound);
            audioSoundLoaded = false;
        }
        currentState = STATE_AUDIO_OPTIONS;
        return;
    }

    
    Rectangle sourcePanel = {50, 100, screenWidth - 100, 60};
    DrawRectangleRounded(sourcePanel, 0.1f, 10, Fade(WHITE, 0.9f));
    DrawRectangleRoundedLines(sourcePanel, 0.1f, 10, accentColor);
    
    char sourceText[100];
    switch (selectedAudioSource) {
        case AUDIO_UPLOAD: strcpy(sourceText, "📂 Source: Uploaded Audio"); break;
        case AUDIO_GENERATE: strcpy(sourceText, "🎶 Source: Generated Audio"); break;
        case AUDIO_YOUTUBE: strcpy(sourceText, "📺 Source: YouTube Audio"); break;
        default: strcpy(sourceText, "❌ Source: None Selected"); break;
    }
    DrawText(sourceText, (int)sourcePanel.x + 20, (int)sourcePanel.y + 20, 18, themeColor);

    int yPos = 180;

    
    if (selectedAudioSource == AUDIO_UPLOAD) {
        DrawText("📁 Audio file path:", 50, yPos, 16, themeColor);
        Rectangle filePathRect = {50, yPos + 25, screenWidth - 200, 35};
        DrawRectangleRounded(filePathRect, 0.1f, 10, WHITE);
        DrawRectangleRoundedLines(filePathRect, 0.1f, 10, filePathEditMode ? accentColor : GRAY);
        
        if (GuiTextBox(filePathRect, filePathBuffer, 256, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
        yPos += 75;
    }

    
    if (selectedAudioSource == AUDIO_YOUTUBE) {
        DrawText("📺 YouTube URL:", 50, yPos, 16, themeColor);
        Rectangle ytRect = {50, yPos + 25, screenWidth - 200, 35};
        DrawRectangleRounded(ytRect, 0.1f, 10, WHITE);
        DrawRectangleRoundedLines(ytRect, 0.1f, 10, ytLinkEditMode ? accentColor : GRAY);
        
        if (GuiTextBox(ytRect, ytLinkBuffer, 256, ytLinkEditMode)) {
            ytLinkEditMode = !ytLinkEditMode;
        }
        
        Rectangle downloadBtn = {screenWidth - 130, yPos + 25, 80, 35};
        bool downloadPressed = CheckCollisionPointRec(GetMousePosition(), downloadBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        DrawGradientButton(downloadBtn, "⬇️ Get", accentColor, Fade(accentColor, 0.8f), downloadPressed);
        
        if (downloadPressed && strlen(ytLinkBuffer) > 0) {
            DownloadFromYouTube(ytLinkBuffer);
        }
        yPos += 75;
    }

    
    DrawText("💬 Enter message to hide:", 50, yPos, 16, themeColor);
    Rectangle messageRect = {50, yPos + 25, screenWidth - 100, 35};
    DrawRectangleRounded(messageRect, 0.1f, 10, WHITE);
    DrawRectangleRoundedLines(messageRect, 0.1f, 10, messageEditMode ? accentColor : GRAY);
    
    if (GuiTextBox(messageRect, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }
    yPos += 75;

    
    const char* audioPath = NULL;
    if (selectedAudioSource == AUDIO_UPLOAD) {
        audioPath = filePathBuffer;
    } else if (selectedAudioSource == AUDIO_GENERATE) {
        audioPath = "randomAudio.wav";
    } else if (selectedAudioSource == AUDIO_YOUTUBE) {
        audioPath = "youtube_audio.wav";
    }

    if (audioPath && FileExists(audioPath)) {
        if (!audioSoundLoaded) {
            audioSound = LoadSound(audioPath);
            audioSoundLoaded = true;
        }

        
        Rectangle audioPanel = {50, yPos, screenWidth - 100, 120};
        DrawRectangleRounded(audioPanel, 0.1f, 10, Fade(WHITE, 0.9f));
        DrawRectangleRoundedLines(audioPanel, 0.1f, 10, accentColor);
        
        DrawText("🎧 Audio Preview:", (int)audioPanel.x + 20, (int)audioPanel.y + 15, 16, themeColor);
        
        Rectangle playBtn = {(int)audioPanel.x + 20, (int)audioPanel.y + 45, 80, 35};
        Rectangle stopBtn = {(int)audioPanel.x + 110, (int)audioPanel.y + 45, 80, 35};
        
        bool playPressed = CheckCollisionPointRec(GetMousePosition(), playBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool stopPressed = CheckCollisionPointRec(GetMousePosition(), stopBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        
        DrawGradientButton(playBtn, "▶️ Play", successColor, Fade(successColor, 0.8f), playPressed);
        DrawGradientButton(stopBtn, "⏹️ Stop", errorColor, Fade(errorColor, 0.8f), stopPressed);
        
        if (playPressed) {
            PlaySound(audioSound);
        }
        if (stopPressed) {
            StopSound(audioSound);
        }

        
        Rectangle encodeBtn = {screenWidth/2 - 70, (int)audioPanel.y + 90, 140, 45};
        bool encodePressed = CheckCollisionPointRec(GetMousePosition(), encodeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        DrawGradientButton(encodeBtn, "🔒 Encode", successColor, Fade(successColor, 0.8f), encodePressed);
        
        if (encodePressed) {
            if (strlen(messageBuffer) > 0) {
                EncodeMessageInAudio(audioPath, messageBuffer, "encoded_audio.wav");
            } else {
                ShowStatusMessage("⚠️ Please enter a message to encode!");
            }
        }
    } else if (selectedAudioSource != AUDIO_NONE) {
        Rectangle errorPanel = {50, yPos, screenWidth - 100, 60};
        DrawRectangleRounded(errorPanel, 0.1f, 10, Fade(errorColor, 0.2f));
        DrawRectangleRoundedLines(errorPanel, 0.1f, 10, errorColor);
        DrawText("❌ Audio file not found! Please check the file path.", (int)errorPanel.x + 20, (int)errorPanel.y + 20, 16, errorColor);
    }

    DrawStatusMessage();
}

void DrawAudioDecode() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){248, 250, 252, 255}, (Color){237, 242, 247, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🔓 Decode Message from Audio";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 28, 24, WHITE);

    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_AUDIO_STEGO;
        return;
    }

    
    DrawText("📁 Audio file path:", 50, 120, 16, themeColor);
    Rectangle filePathRect = {50, 145, screenWidth - 100, 35};
    DrawRectangleRounded(filePathRect, 0.1f, 10, WHITE);
    DrawRectangleRoundedLines(filePathRect, 0.1f, 10, filePathEditMode ? accentColor : GRAY);
    
    if (GuiTextBox(filePathRect, filePathBuffer, 256, filePathEditMode)) {
        filePathEditMode = !filePathEditMode;
    }

    
    Rectangle decodeBtn = {screenWidth/2 - 70, 200, 140, 45};
    bool decodePressed = CheckCollisionPointRec(GetMousePosition(), decodeBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(decodeBtn, "🔍 Decode", successColor, Fade(successColor, 0.8f), decodePressed);
    
    if (decodePressed) {
        if (strlen(filePathBuffer) > 0) {
            char* decoded = DecodeMessageFromAudio(filePathBuffer);
            if (decoded) {
                strncpy(decodedMessageBuffer, decoded, sizeof(decodedMessageBuffer) - 1);
                decodedMessageBuffer[sizeof(decodedMessageBuffer) - 1] = '\0';
                free(decoded);
                ShowStatusMessage("✅ Message decoded successfully!");
            }
        } else {
            ShowStatusMessage("⚠️ Please enter a file path!");
        }
    }

    
    if (strlen(decodedMessageBuffer) > 0) {
        Rectangle messagePanel = {50, 270, screenWidth - 100, 150};
        DrawRectangleRounded(messagePanel, 0.1f, 10, Fade(WHITE, 0.9f));
        DrawRectangleRoundedLines(messagePanel, 0.1f, 10, successColor);
        
        DrawText("📝 Decoded Message:", (int)messagePanel.x + 20, (int)messagePanel.y + 15, 16, themeColor);
        
        
        int maxWidth = (int)messagePanel.width - 40;
        int lineHeight = 20;
        int currentY = (int)messagePanel.y + 45;
        
        char* words = strdup(decodedMessageBuffer);
        char* word = strtok(words, " ");
        char currentLine[256] = "";
        
        while (word != NULL) {
            char tempLine[256];
            snprintf(tempLine, sizeof(tempLine), "%s%s%s", currentLine, strlen(currentLine) > 0 ? " " : "", word);
            
            if (MeasureText(tempLine, 16) <= maxWidth) {
                strcpy(currentLine, tempLine);
            } else {
                if (strlen(currentLine) > 0) {
                    DrawText(currentLine, (int)messagePanel.x + 20, currentY, 16, themeColor);
                    currentY += lineHeight;
                }
                strcpy(currentLine, word);
            }
            word = strtok(NULL, " ");
        }
        
        if (strlen(currentLine) > 0) {
            DrawText(currentLine, (int)messagePanel.x + 20, currentY, 16, themeColor);
        }
        
        free(words);
    }

    DrawStatusMessage();
}

void DrawMainMenu() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){20, 30, 48, 255}, (Color){36, 59, 85, 255});
    
    
    static float time = 0;
    time += GetFrameTime();
    
    for (int i = 0; i < 20; i++) {
        float x = (float)(i * 100 + (int)(sin(time + i) * 50));
        float y = (float)(100 + i * 30 + (int)(cos(time + i * 0.5f) * 30));
        DrawCircle((int)x, (int)y, 3, Fade(accentColor, 0.3f));
    }
    
    
    const char* title = "StegaNet";
    const char* subtitle = "Advanced Steganography Suite";
    
    int titleWidth = MeasureText(title, 60);
    int subtitleWidth = MeasureText(subtitle, 24);
    
    
    for (int i = 0; i < 5; i++) {
        DrawText(title, screenWidth/2 - titleWidth/2 + i, 120 + i, 60, Fade(accentColor, 0.1f));
        DrawText(title, screenWidth/2 - titleWidth/2 - i, 120 - i, 60, Fade(accentColor, 0.1f));
    }
    DrawText(title, screenWidth/2 - titleWidth/2, 120, 60, WHITE);
    DrawText(subtitle, screenWidth/2 - subtitleWidth/2, 200, 24, Fade(WHITE, 0.8f));

    
    Rectangle imageCard = {150, 280, 280, 120};
    Rectangle audioCard = {570, 280, 280, 120};
    
    bool imageHover = CheckCollisionPointRec(GetMousePosition(), imageCard);
    bool audioHover = CheckCollisionPointRec(GetMousePosition(), audioCard);
    bool imagePressed = imageHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool audioPressed = audioHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    
    Color imageCardColor = imageHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(imageCard, 0.1f, 10, imageCardColor);
    DrawRectangleRoundedLines(imageCard, 0.1f, 10, accentColor);
    
    DrawText("🖼️", (int)imageCard.x + 20, (int)imageCard.y + 20, 40, themeColor);
    DrawText("Image Steganography", (int)imageCard.x + 80, (int)imageCard.y + 25, 20, themeColor);
    DrawText("Hide messages in images", (int)imageCard.x + 20, (int)imageCard.y + 60, 16, Fade(themeColor, 0.7f));
    DrawText("• Upload or generate images", (int)imageCard.x + 20, (int)imageCard.y + 80, 14, Fade(themeColor, 0.6f));
    DrawText("• LSB encoding technique", (int)imageCard.x + 20, (int)imageCard.y + 95, 14, Fade(themeColor, 0.6f));
    
    
    Color audioCardColor = audioHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(audioCard, 0.1f, 10, audioCardColor);
    DrawRectangleRoundedLines(audioCard, 0.1f, 10, accentColor);
    
    DrawText("🎵", (int)audioCard.x + 20, (int)audioCard.y + 20, 40, themeColor);
    DrawText("Audio Steganography", (int)audioCard.x + 80, (int)audioCard.y + 25, 20, themeColor);
    DrawText("Hide messages in audio", (int)audioCard.x + 20, (int)audioCard.y + 60, 16, Fade(themeColor, 0.7f));
    DrawText("• Upload, generate, or YouTube", (int)audioCard.x + 20, (int)audioCard.y + 80, 14, Fade(themeColor, 0.6f));
    DrawText("• LSB audio encoding", (int)audioCard.x + 20, (int)audioCard.y + 95, 14, Fade(themeColor, 0.6f));
    
    if (imagePressed) {
        currentState = STATE_IMAGE_STEGO;
    }
    if (audioPressed) {
        currentState = STATE_AUDIO_STEGO;
    }

    
    Rectangle exitBtn = {screenWidth/2 - 60, 450, 120, 50};
    bool exitHover = CheckCollisionPointRec(GetMousePosition(), exitBtn);
    bool exitPressed = exitHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    DrawGradientButton(exitBtn, "🚪 Exit", errorColor, Fade(errorColor, 0.8f), exitPressed);
    
    if (exitPressed) {
        
        if (encodedTextureLoaded) UnloadTexture(encodedTexture);
        if (audioWaveLoaded) UnloadWave(audioWave);
        if (audioSoundLoaded) UnloadSound(audioSound);
        CloseWindow();
        exit(0);
    }

    DrawStatusMessage();
}

void DrawImageSteganography() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){240, 248, 255, 255}, (Color){225, 245, 254, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🖼️ Image Steganography";
    int titleWidth = MeasureText(title, 28);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 26, 28, WHITE);
    
    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_AUDIO_STEGO;
    }
    
    
    Rectangle uploadCard = {80, 150, 200, 200};
    Rectangle generateCard = {300, 150, 200, 200};
    Rectangle youtubeCard = {520, 150, 200, 200};
    
    bool uploadHover = CheckCollisionPointRec(GetMousePosition(), uploadCard);
    bool generateHover = CheckCollisionPointRec(GetMousePosition(), generateCard);
    bool youtubeHover = CheckCollisionPointRec(GetMousePosition(), youtubeCard);
    bool uploadPressed = uploadHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool generatePressed = generateHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool youtubePressed = youtubeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    
    Color uploadCardColor = uploadHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(uploadCard, 0.1f, 10, uploadCardColor);
    DrawRectangleRoundedLines(uploadCard, 0.1f, 10, accentColor);
    
    DrawText("📂", (int)uploadCard.x + 20, (int)uploadCard.y + 20, 40, themeColor);
    DrawText("Upload", (int)uploadCard.x + 80, (int)uploadCard.y + 30, 18, themeColor);
    DrawText("Audio", (int)uploadCard.x + 80, (int)uploadCard.y + 50, 18, themeColor);
    DrawText("Use your own", (int)uploadCard.x + 20, (int)uploadCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("audio file", (int)uploadCard.x + 20, (int)uploadCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• WAV, MP3", (int)uploadCard.x + 20, (int)uploadCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• High quality", (int)uploadCard.x + 20, (int)uploadCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Large capacity", (int)uploadCard.x + 20, (int)uploadCard.y + 170, 12, Fade(themeColor, 0.6f));
    
    
    Color generateCardColor = generateHover ? Fade(successColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(generateCard, 0.1f, 10, generateCardColor);
    DrawRectangleRoundedLines(generateCard, 0.1f, 10, successColor);
    
    DrawText("🎶", (int)generateCard.x + 20, (int)generateCard.y + 20, 40, themeColor);
    DrawText("Generate", (int)generateCard.x + 80, (int)generateCard.y + 30, 18, themeColor);
    DrawText("Audio", (int)generateCard.x + 80, (int)generateCard.y + 50, 18, themeColor);
    DrawText("Create musical", (int)generateCard.x + 20, (int)generateCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("pattern", (int)generateCard.x + 20, (int)generateCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• Multi-frequency", (int)generateCard.x + 20, (int)generateCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• 10 seconds", (int)generateCard.x + 20, (int)generateCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Ready to use", (int)generateCard.x + 20, (int)generateCard.y + 170, 12, Fade(themeColor, 0.6f));
    
    
    Color youtubeCardColor = youtubeHover ? Fade(errorColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(youtubeCard, 0.1f, 10, youtubeCardColor);
    DrawRectangleRoundedLines(youtubeCard, 0.1f, 10, errorColor);
    
    DrawText("📺", (int)youtubeCard.x + 20, (int)youtubeCard.y + 20, 40, themeColor);
    DrawText("YouTube", (int)youtubeCard.x + 80, (int)youtubeCard.y + 30, 18, themeColor);
    DrawText("Link", (int)youtubeCard.x + 80, (int)youtubeCard.y + 50, 18, themeColor);
    DrawText("Download from", (int)youtubeCard.x + 20, (int)youtubeCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("YouTube URL", (int)youtubeCard.x + 20, (int)youtubeCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• Any video", (int)youtubeCard.x + 20, (int)youtubeCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• Auto extract", (int)youtubeCard.x + 20, (int)youtubeCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Requires yt-dlp", (int)youtubeCard.x + 20, (int)youtubeCard.y + 170, 12, Fade(themeColor, 0.6f));

    if (uploadPressed) {
        selectedAudioSource = AUDIO_UPLOAD;
        filePathBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }
    if (generatePressed) {
        selectedAudioSource = AUDIO_GENERATE;
        messageBuffer[0] = '\0';
        GenerateRandomAudio();
        currentState = STATE_AUDIO_ENCODE;
    }
    if (youtubePressed) {
        selectedAudioSource = AUDIO_YOUTUBE;
        ytLinkBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }

    
    const char* desc = "Choose how you want to provide the audio for encoding";
    int descWidth = MeasureText(desc, 18);
    DrawText(desc, screenWidth / 2 - descWidth / 2, 380, 18, Fade(themeColor, 0.8f));

    DrawStatusMessage();
}

void DrawAudioSteganography() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){248, 250, 252, 255}, (Color){237, 242, 247, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🎵 Audio Steganography";
    int titleWidth = MeasureText(title, 28);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 26, 28, WHITE);

    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_MAIN_MENU;
    }

    
    Rectangle encodeCard = {150, 200, 280, 160};
    Rectangle decodeCard = {570, 200, 280, 160};
    
    bool encodeHover = CheckCollisionPointRec(GetMousePosition(), encodeCard);
    bool decodeHover = CheckCollisionPointRec(GetMousePosition(), decodeCard);
    bool encodePressed = encodeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool decodePressed = decodeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    
    Color encodeCardColor = encodeHover ? Fade(successColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(encodeCard, 0.1f, 10, encodeCardColor);
    DrawRectangleRoundedLines(encodeCard, 0.1f, 10, successColor);
    
    DrawText("🔒", (int)encodeCard.x + 20, (int)encodeCard.y + 20, 50, themeColor);
    DrawText("Encode Message", (int)encodeCard.x + 90, (int)encodeCard.y + 30, 22, themeColor);
    DrawText("Hide your secret message", (int)encodeCard.x + 20, (int)encodeCard.y + 70, 16, Fade(themeColor, 0.7f));
    DrawText("inside an audio file", (int)encodeCard.x + 20, (int)encodeCard.y + 90, 16, Fade(themeColor, 0.7f));
    DrawText("• Choose audio source", (int)encodeCard.x + 20, (int)encodeCard.y + 115, 14, Fade(themeColor, 0.6f));
    DrawText("• Enter your message", (int)encodeCard.x + 20, (int)encodeCard.y + 135, 14, Fade(themeColor, 0.6f));
    
    
    Color decodeCardColor = decodeHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(decodeCard, 0.1f, 10, decodeCardColor);
    DrawRectangleRoundedLines(decodeCard, 0.1f, 10, accentColor);
    
    DrawText("🔓", (int)decodeCard.x + 20, (int)decodeCard.y + 20, 50, themeColor);
    DrawText("Decode Message", (int)decodeCard.x + 90, (int)decodeCard.y + 30, 22, themeColor);
    DrawText("Extract hidden message", (int)decodeCard.x + 20, (int)decodeCard.y + 70, 16, Fade(themeColor, 0.7f));
    DrawText("from an encoded audio", (int)decodeCard.x + 20, (int)decodeCard.y + 90, 16, Fade(themeColor, 0.7f));
    DrawText("• Select encoded audio", (int)decodeCard.x + 20, (int)decodeCard.y + 115, 14, Fade(themeColor, 0.6f));
    DrawText("• Reveal hidden message", (int)decodeCard.x + 20, (int)decodeCard.y + 135, 14, Fade(themeColor, 0.6f));

    if (encodePressed) {
        currentState = STATE_AUDIO_OPTIONS;
        selectedAudioSource = AUDIO_NONE;
    }
    if (decodePressed) {
        currentState = STATE_AUDIO_DECODE;
        filePathBuffer[0] = '\0';
        decodedMessageBuffer[0] = '\0';
    }

    
    const char* desc = "Select an option to encode or decode messages in audio files";
    int descWidth = MeasureText(desc, 18);
    DrawText(desc, screenWidth / 2 - descWidth / 2, 400, 18, Fade(themeColor, 0.8f));

    DrawStatusMessage();
}


void DrawImageOptions() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){240, 248, 255, 255}, (Color){225, 245, 254, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🖼️ Choose Image Source";
    int titleWidth = MeasureText(title, 28);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 26, 28, WHITE);
    
    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_IMAGE_STEGO;
    }
    
    
    Rectangle uploadCard = {120, 180, 240, 180};
    Rectangle generateCard = {400, 180, 240, 180};
    
    bool uploadHover = CheckCollisionPointRec(GetMousePosition(), uploadCard);
    bool generateHover = CheckCollisionPointRec(GetMousePosition(), generateCard);
    bool uploadPressed = uploadHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool generatePressed = generateHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    
    Color uploadCardColor = uploadHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(uploadCard, 0.1f, 10, uploadCardColor);
    DrawRectangleRoundedLines(uploadCard, 0.1f, 10, accentColor);
    
    DrawText("📂", (int)uploadCard.x + 20, (int)uploadCard.y + 20, 50, themeColor);
    DrawText("Upload Image", (int)uploadCard.x + 90, (int)uploadCard.y + 35, 20, themeColor);
    DrawText("Use your own image file", (int)uploadCard.x + 20, (int)uploadCard.y + 80, 16, Fade(themeColor, 0.7f));
    DrawText("• PNG, JPG, BMP supported", (int)uploadCard.x + 20, (int)uploadCard.y + 105, 14, Fade(themeColor, 0.6f));
    DrawText("• Better capacity for", (int)uploadCard.x + 20, (int)uploadCard.y + 125, 14, Fade(themeColor, 0.6f));
    DrawText("  larger images", (int)uploadCard.x + 20, (int)uploadCard.y + 145, 14, Fade(themeColor, 0.6f));
    
    
    Color generateCardColor = generateHover ? Fade(successColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(generateCard, 0.1f, 10, generateCardColor);
    DrawRectangleRoundedLines(generateCard, 0.1f, 10, successColor);
    
    DrawText("🎨", (int)generateCard.x + 20, (int)generateCard.y + 20, 50, themeColor);
    DrawText("Generate Image", (int)generateCard.x + 90, (int)generateCard.y + 35, 20, themeColor);
    DrawText("Create a random pattern", (int)generateCard.x + 20, (int)generateCard.y + 80, 16, Fade(themeColor, 0.7f));
    DrawText("• Fractal-based design", (int)generateCard.x + 20, (int)generateCard.y + 105, 14, Fade(themeColor, 0.6f));
    DrawText("• 512x512 resolution", (int)generateCard.x + 20, (int)generateCard.y + 125, 14, Fade(themeColor, 0.6f));
    DrawText("• Visually appealing", (int)generateCard.x + 20, (int)generateCard.y + 145, 14, Fade(themeColor, 0.6f));
    
    if (uploadPressed) {
        selectedImageSource = IMAGE_UPLOAD;
        filePathBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_IMAGE_ENCODE;
    }
    if (generatePressed) {
        selectedImageSource = IMAGE_GENERATE;
        messageBuffer[0] = '\0';
        GenerateRandomImage();
        currentState = STATE_IMAGE_ENCODE;
    }
    
    
    const char* desc = "Choose how you want to provide the image for encoding";
    int descWidth = MeasureText(desc, 18);
    DrawText(desc, screenWidth / 2 - descWidth / 2, 400, 18, Fade(themeColor, 0.8f));

    DrawStatusMessage();
}

void DrawAudioOptions() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                          (Color){248, 250, 252, 255}, (Color){237, 242, 247, 255});
    DrawBackgroundPattern(screenWidth, screenHeight);

    
    DrawRectangle(0, 0, screenWidth, 80, themeColor);
    const char* title = "🎵 Choose Audio Source";
    int titleWidth = MeasureText(title, 28);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 26, 28, WHITE);
    
    
    Rectangle backBtn = {20, 20, 80, 40};
    bool backPressed = CheckCollisionPointRec(GetMousePosition(), backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    DrawGradientButton(backBtn, "← Back", accentColor, Fade(accentColor, 0.8f), backPressed);
    if (backPressed) {
        currentState = STATE_AUDIO_STEGO;
    }
    
    
    Rectangle uploadCard = {80, 150, 200, 200};
    Rectangle generateCard = {300, 150, 200, 200};
    Rectangle youtubeCard = {520, 150, 200, 200};
    
    bool uploadHover = CheckCollisionPointRec(GetMousePosition(), uploadCard);
    bool generateHover = CheckCollisionPointRec(GetMousePosition(), generateCard);
    bool youtubeHover = CheckCollisionPointRec(GetMousePosition(), youtubeCard);
    bool uploadPressed = uploadHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool generatePressed = generateHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool youtubePressed = youtubeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    
    
    Color uploadCardColor = uploadHover ? Fade(accentColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(uploadCard, 0.1f, 10, uploadCardColor);
    DrawRectangleRoundedLines(uploadCard, 0.1f, 10, accentColor);
    
    DrawText("📂", (int)uploadCard.x + 20, (int)uploadCard.y + 20, 40, themeColor);
    DrawText("Upload", (int)uploadCard.x + 80, (int)uploadCard.y + 30, 18, themeColor);
    DrawText("Audio", (int)uploadCard.x + 80, (int)uploadCard.y + 50, 18, themeColor);
    DrawText("Use your own", (int)uploadCard.x + 20, (int)uploadCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("audio file", (int)uploadCard.x + 20, (int)uploadCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• WAV, MP3", (int)uploadCard.x + 20, (int)uploadCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• High quality", (int)uploadCard.x + 20, (int)uploadCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Large capacity", (int)uploadCard.x + 20, (int)uploadCard.y + 170, 12, Fade(themeColor, 0.6f));
    
    
    Color generateCardColor = generateHover ? Fade(successColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(generateCard, 0.1f, 10, generateCardColor);
    DrawRectangleRoundedLines(generateCard, 0.1f, 10, successColor);
    
    DrawText("🎶", (int)generateCard.x + 20, (int)generateCard.y + 20, 40, themeColor);
    DrawText("Generate", (int)generateCard.x + 80, (int)generateCard.y + 30, 18, themeColor);
    DrawText("Audio", (int)generateCard.x + 80, (int)generateCard.y + 50, 18, themeColor);
    DrawText("Create musical", (int)generateCard.x + 20, (int)generateCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("pattern", (int)generateCard.x + 20, (int)generateCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• Multi-frequency", (int)generateCard.x + 20, (int)generateCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• 10 seconds", (int)generateCard.x + 20, (int)generateCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Ready to use", (int)generateCard.x + 20, (int)generateCard.y + 170, 12, Fade(themeColor, 0.6f));
    
    
    Color youtubeCardColor = youtubeHover ? Fade(errorColor, 0.8f) : Fade(WHITE, 0.9f);
    DrawRectangleRounded(youtubeCard, 0.1f, 10, youtubeCardColor);
    DrawRectangleRoundedLines(youtubeCard, 0.1f, 10, errorColor);
    
    DrawText("📺", (int)youtubeCard.x + 20, (int)youtubeCard.y + 20, 40, themeColor);
    DrawText("YouTube", (int)youtubeCard.x + 80, (int)youtubeCard.y + 30, 18, themeColor);
    DrawText("Link", (int)youtubeCard.x + 80, (int)youtubeCard.y + 50, 18, themeColor);
    DrawText("Download from", (int)youtubeCard.x + 20, (int)youtubeCard.y + 85, 14, Fade(themeColor, 0.7f));
    DrawText("YouTube URL", (int)youtubeCard.x + 20, (int)youtubeCard.y + 105, 14, Fade(themeColor, 0.7f));
    DrawText("• Any video", (int)youtubeCard.x + 20, (int)youtubeCard.y + 130, 12, Fade(themeColor, 0.6f));
    DrawText("• Auto extract", (int)youtubeCard.x + 20, (int)youtubeCard.y + 150, 12, Fade(themeColor, 0.6f));
    DrawText("• Requires yt-dlp", (int)youtubeCard.x + 20, (int)youtubeCard.y + 170, 12, Fade(themeColor, 0.6f));

    if (uploadPressed) {
        selectedAudioSource = AUDIO_UPLOAD;
        filePathBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }
    if (generatePressed) {
        selectedAudioSource = AUDIO_GENERATE;
        messageBuffer[0] = '\0';
        GenerateRandomAudio();
        currentState = STATE_AUDIO_ENCODE;
    }
    if (youtubePressed) {
        selectedAudioSource = AUDIO_YOUTUBE;
        ytLinkBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }

    
    const char* desc = "Choose how you want to provide the audio for encoding";
    int descWidth = MeasureText(desc, 18);
    DrawText(desc, screenWidth / 2 - descWidth / 2, 380, 18, Fade(themeColor, 0.8f));

    DrawStatusMessage();
}

int main(void) {
    InitWindow(1000, 650, "StegaNet - Advanced Steganography Suite");
    InitAudioDevice();
    SetTargetFPS(60);

    
    SetRandomSeed((unsigned int)time(NULL));

    
    
    
    

    while (!WindowShouldClose()) {
        UpdateStatusMessage();

        BeginDrawing();

        switch (currentState) {
            case STATE_MAIN_MENU:
                DrawMainMenu();
                break;
            case STATE_IMAGE_STEGO:
                DrawImageSteganography();
                break;
            case STATE_AUDIO_STEGO:
                DrawAudioSteganography();
                break;
            case STATE_IMAGE_OPTIONS:
                DrawImageOptions();
                break;
            case STATE_AUDIO_OPTIONS:
                DrawAudioOptions();
                break;
            case STATE_IMAGE_ENCODE:
                DrawImageEncode();
                break;
            case STATE_IMAGE_DECODE:
                DrawImageDecode();
                break;
            case STATE_AUDIO_ENCODE:
                DrawAudioEncode();
                break;
            case STATE_AUDIO_DECODE:
                DrawAudioDecode();
                break;
        }

        EndDrawing();
    }

    
    if (encodedTextureLoaded) UnloadTexture(encodedTexture);
    if (audioWaveLoaded) UnloadWave(audioWave);
    if (audioSoundLoaded) UnloadSound(audioSound);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
