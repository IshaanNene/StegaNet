#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Application states
typedef enum {
    STATE_MAIN_MENU,
    STATE_IMAGE_STEGO,
    STATE_AUDIO_STEGO,
    STATE_IMAGE_OPTIONS,
    STATE_AUDIO_OPTIONS,
    STATE_IMAGE_ENCODE,
    STATE_IMAGE_DECODE,
    STATE_AUDIO_ENCODE,
    STATE_AUDIO_DECODE,
    STATE_YOUTUBE_INPUT
} AppState;

// Image source options
typedef enum {
    IMAGE_UPLOAD,
    IMAGE_GENERATE,
    IMAGE_NONE
} ImageSource;

// Audio source options
typedef enum {
    AUDIO_UPLOAD,
    AUDIO_GENERATE,
    AUDIO_YOUTUBE,
    AUDIO_NONE
} AudioSource;

// Global state variables
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

// Utility functions
void ShowStatusMessage(const char* message) {
    strncpy(statusMessage, message, sizeof(statusMessage) - 1);
    statusMessage[sizeof(statusMessage) - 1] = '\0';
    statusMessageTimer = 3.0f; // Show for 3 seconds
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
        DrawText(statusMessage, screenWidth / 2 - textWidth / 2, 30, 16, BLUE);
    }
}

// Image steganography functions
void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating random image...");
    
    // Generate a simple random image
    int width = 512;
    int height = 512;
    Image image = GenImageColor(width, height, WHITE);
    
    // Add some random patterns
    for (int i = 0; i < 1000; i++) {
        int x = GetRandomValue(0, width - 1);
        int y = GetRandomValue(0, height - 1);
        Color color = {
            GetRandomValue(0, 255),
            GetRandomValue(0, 255),
            GetRandomValue(0, 255),
            255
        };
        ImageDrawPixel(&image, x, y, color);
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

// Audio steganography functions
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

// Drawing functions
void DrawImageEncode() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    // Back button
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
        if (encodedTextureLoaded) {
            UnloadTexture(encodedTexture);
            encodedTextureLoaded = false;
        }
        currentState = STATE_IMAGE_OPTIONS;
        return;
    }

    // Title
    const char* title = "Encode Message in Image";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    // Source info
    char sourceText[100];
    switch (selectedImageSource) {
        case IMAGE_UPLOAD: 
            strcpy(sourceText, "Source: Uploaded Image"); 
            break;
        case IMAGE_GENERATE: 
            strcpy(sourceText, "Source: Generated Image"); 
            break;
        default: 
            strcpy(sourceText, "Source: None"); 
            break;
    }
    DrawText(sourceText, screenWidth / 2 - MeasureText(sourceText, 18) / 2, 100, 18, DARKGRAY);

    // File path input for uploaded images
    if (selectedImageSource == IMAGE_UPLOAD) {
        DrawText("Image file path:", screenWidth / 2 - 200, 120, 16, DARKGRAY);
        if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 140, 400, 30 }, filePathBuffer, 256, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
    }

    // Message input
    DrawText("Enter message to hide:", screenWidth / 2 - 200, 180, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 200, 400, 30 }, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }

    // Load and display image
    const char* imagePath = (selectedImageSource == IMAGE_UPLOAD) ? filePathBuffer : "randomImage.png";
    
    if (!encodedTextureLoaded) {
        if (FileExists(imagePath)) {
            encodedTexture = LoadTexture(imagePath);
            encodedTextureLoaded = true;
        } else {
            DrawText("Image not found! Please check the file path.", screenWidth / 2 - 200, 280, 16, RED);
        }
    }

    if (encodedTextureLoaded) {
        float imageY = 250;
        float scale = 200.0f / fmax(encodedTexture.width, encodedTexture.height);
        DrawTextureEx(encodedTexture, 
                     (Vector2){ screenWidth / 2 - (encodedTexture.width * scale) / 2, imageY }, 
                     0, scale, WHITE);

        // Buttons
        float buttonY = imageY + (encodedTexture.height * scale) + 20;
        if (GuiButton((Rectangle){ screenWidth / 2 - 160, buttonY, 140, 40 }, "Encode")) {
            if (strlen(messageBuffer) > 0) {
                EncodeMessageInImage(imagePath, messageBuffer, "encoded_image.png");
            } else {
                ShowStatusMessage("Please enter a message to encode!");
            }
        }

        if (GuiButton((Rectangle){ screenWidth / 2 + 20, buttonY, 140, 40 }, "Save As...")) {
            ShowStatusMessage("Encoded image saved as 'encoded_image.png'");
        }
    }

    DrawStatusMessage();
}

void DrawImageDecode() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    // Back button
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
        currentState = STATE_IMAGE_STEGO;
        return;
    }

    // Title
    const char* title = "Decode Message from Image";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    // File path input
    DrawText("Image file path:", screenWidth / 2 - 200, 120, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 140, 400, 30 }, filePathBuffer, 256, filePathEditMode)) {
        filePathEditMode = !filePathEditMode;
    }

    // Decode button
    if (GuiButton((Rectangle){ screenWidth / 2 - 70, 190, 140, 40 }, "Decode")) {
        if (strlen(filePathBuffer) > 0) {
            char* decoded = DecodeMessageFromImage(filePathBuffer);
            if (decoded) {
                strncpy(decodedMessageBuffer, decoded, sizeof(decodedMessageBuffer) - 1);
                decodedMessageBuffer[sizeof(decodedMessageBuffer) - 1] = '\0';
                free(decoded);
                ShowStatusMessage("Message decoded successfully!");
            }
        } else {
            ShowStatusMessage("Please enter a file path!");
        }
    }

    // Display decoded message
    if (strlen(decodedMessageBuffer) > 0) {
        DrawText("Decoded message:", screenWidth / 2 - 200, 260, 16, DARKGRAY);
        DrawRectangle(screenWidth / 2 - 200, 280, 400, 100, LIGHTGRAY);
        DrawRectangleLines(screenWidth / 2 - 200, 280, 400, 100, GRAY);
        DrawText(decodedMessageBuffer, screenWidth / 2 - 190, 290, 16, DARKGRAY);
    }

    DrawStatusMessage();
}

void DrawAudioEncode() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    // Back button
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
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

    // Title
    const char* title = "Encode Message in Audio";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    // Source info
    char sourceText[100];
    switch (selectedAudioSource) {
        case AUDIO_UPLOAD: 
            strcpy(sourceText, "Source: Uploaded Audio"); 
            break;
        case AUDIO_GENERATE: 
            strcpy(sourceText, "Source: Generated Audio"); 
            break;
        case AUDIO_YOUTUBE: 
            strcpy(sourceText, "Source: YouTube Audio"); 
            break;
        default: 
            strcpy(sourceText, "Source: None"); 
            break;
    }
    DrawText(sourceText, screenWidth / 2 - MeasureText(sourceText, 18) / 2, 100, 18, DARKGRAY);

    // File path input for uploaded audio
    if (selectedAudioSource == AUDIO_UPLOAD) {
        DrawText("Audio file path:", screenWidth / 2 - 200, 120, 16, DARKGRAY);
        if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 140, 400, 30 }, filePathBuffer, 256, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
    }

    // YouTube link input
    if (selectedAudioSource == AUDIO_YOUTUBE) {
        DrawText("YouTube URL:", screenWidth / 2 - 200, 120, 16, DARKGRAY);
        if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 140, 400, 30 }, ytLinkBuffer, 256, ytLinkEditMode)) {
            ytLinkEditMode = !ytLinkEditMode;
        }
        if (GuiButton((Rectangle){ screenWidth / 2 + 210, 140, 80, 30 }, "Download")) {
            if (strlen(ytLinkBuffer) > 0) {
                DownloadFromYouTube(ytLinkBuffer);
            }
        }
    }

    // Message input
    DrawText("Enter message to hide:", screenWidth / 2 - 200, 180, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 200, 400, 30 }, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }

    // Audio controls
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

        // Audio controls
        DrawText("Audio preview:", screenWidth / 2 - 200, 250, 16, DARKGRAY);
        if (GuiButton((Rectangle){ screenWidth / 2 - 200, 270, 80, 30 }, "Play")) {
            PlaySound(audioSound);
        }
        if (GuiButton((Rectangle){ screenWidth / 2 - 110, 270, 80, 30 }, "Stop")) {
            StopSound(audioSound);
        }

        // Encode button
        if (GuiButton((Rectangle){ screenWidth / 2 - 70, 320, 140, 40 }, "Encode")) {
            if (strlen(messageBuffer) > 0) {
                EncodeMessageInAudio(audioPath, messageBuffer, "encoded_audio.wav");
            } else {
                ShowStatusMessage("Please enter a message to encode!");
            }
        }
    } else {
        DrawText("Audio file not found!", screenWidth / 2 - 100, 250, 16, RED);
    }

    DrawStatusMessage();
}

void DrawAudioDecode() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    // Back button
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
        currentState = STATE_AUDIO_STEGO;
        return;
    }

    // Title
    const char* title = "Decode Message from Audio";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    // File path input
    DrawText("Audio file path:", screenWidth / 2 - 200, 120, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 140, 400, 30 }, filePathBuffer, 256, filePathEditMode)) {
        filePathEditMode = !filePathEditMode;
    }

    // Decode button
    if (GuiButton((Rectangle){ screenWidth / 2 - 70, 190, 140, 40 }, "Decode")) {
        if (strlen(filePathBuffer) > 0) {
            char* decoded = DecodeMessageFromAudio(filePathBuffer);
            if (decoded) {
                strncpy(decodedMessageBuffer, decoded, sizeof(decodedMessageBuffer) - 1);
                decodedMessageBuffer[sizeof(decodedMessageBuffer) - 1] = '\0';
                free(decoded);
                ShowStatusMessage("Message decoded successfully!");
            }
        } else {
            ShowStatusMessage("Please enter a file path!");
        }
    }

    // Display decoded message
    if (strlen(decodedMessageBuffer) > 0) {
        DrawText("Decoded message:", screenWidth / 2 - 200, 260, 16, DARKGRAY);
        DrawRectangle(screenWidth / 2 - 200, 280, 400, 100, LIGHTGRAY);
        DrawRectangleLines(screenWidth / 2 - 200, 280, 400, 100, GRAY);
        DrawText(decodedMessageBuffer, screenWidth / 2 - 190, 290, 16, DARKGRAY);
    }

    DrawStatusMessage();
}

void DrawMainMenu() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    
    // Title
    const char* title = "StegaNet";
    DrawText(title, screenWidth / 2 - MeasureText(title, 40) / 2, 100, 40, DARKGRAY);
    
    // Subtitle
    const char* subtitle = "Choose Steganography Type";
    DrawText(subtitle, screenWidth / 2 - MeasureText(subtitle, 20) / 2, 160, 20, GRAY);

    // Buttons
    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Image Steganography")) {
        currentState = STATE_IMAGE_STEGO;
    }
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Audio Steganography")) {
        currentState = STATE_AUDIO_STEGO;
    }
    if (GuiButton((Rectangle){ 450, 400, 100, 50 }, "Exit")) {
        // Clean up and exit
        if (encodedTextureLoaded) UnloadTexture(encodedTexture);
        if (audioWaveLoaded) UnloadWave(audioWave);
        if (audioSoundLoaded) UnloadSound(audioSound);
        CloseWindow();
        exit(0);
    }

    DrawStatusMessage();
}

void DrawImageSteganography() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    
    // Title
    const char* title = "Image Steganography";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 100, 30, DARKGRAY);
    
    // Back button
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) {
        currentState = STATE_MAIN_MENU;
    }
    
    // Options
    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Encode Message")) {
        currentState = STATE_IMAGE_OPTIONS;
        selectedImageSource = IMAGE_NONE;
    }
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Decode Message")) {
        currentState = STATE_IMAGE_DECODE;
        filePathBuffer[0] = '\0';
        decodedMessageBuffer[0] = '\0';
    }
    
    // Description
    const char* desc = "Select an option to encode or decode messages in audio files";
    DrawText(desc, screenWidth / 2 - MeasureText(desc, 16) / 2, 400, 16, GRAY);

    DrawStatusMessage();
}

void DrawImageOptions() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    
    // Title
    const char* title = "Image Source Options";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 100, 30, DARKGRAY);
    
    // Back button
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) {
        currentState = STATE_IMAGE_STEGO;
    }
    
    // Instructions
    const char* instructions = "Choose image source:";
    DrawText(instructions, screenWidth / 2 - MeasureText(instructions, 20) / 2, 180, 20, DARKGRAY);

    // Options
    if (GuiButton((Rectangle){ 150, 220, 200, 60 }, "Upload Image")) {
        selectedImageSource = IMAGE_UPLOAD;
        filePathBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_IMAGE_ENCODE;
    }
    if (GuiButton((Rectangle){ 400, 220, 200, 60 }, "Generate Random")) {
        selectedImageSource = IMAGE_GENERATE;
        messageBuffer[0] = '\0';
        GenerateRandomImage();
        currentState = STATE_IMAGE_ENCODE;
    }
    
    // Description
    const char* desc = "Upload an existing image or generate a random one";
    DrawText(desc, screenWidth / 2 - MeasureText(desc, 16) / 2, 320, 16, GRAY);

    DrawStatusMessage();
}

void DrawAudioOptions() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    
    // Title
    const char* title = "Audio Source Options";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 100, 30, DARKGRAY);
    
    // Back button
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) {
        currentState = STATE_AUDIO_STEGO;
    }
    
    // Instructions
    const char* instructions = "Choose audio source:";
    DrawText(instructions, screenWidth / 2 - MeasureText(instructions, 20) / 2, 180, 20, DARKGRAY);

    // Options
    if (GuiButton((Rectangle){ 100, 220, 180, 60 }, "Upload Audio")) {
        selectedAudioSource = AUDIO_UPLOAD;
        filePathBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }
    if (GuiButton((Rectangle){ 300, 220, 180, 60 }, "Generate Audio")) {
        selectedAudioSource = AUDIO_GENERATE;
        messageBuffer[0] = '\0';
        GenerateRandomAudio();
        currentState = STATE_AUDIO_ENCODE;
    }
    if (GuiButton((Rectangle){ 500, 220, 180, 60 }, "YouTube Link")) {
        selectedAudioSource = AUDIO_YOUTUBE;
        ytLinkBuffer[0] = '\0';
        messageBuffer[0] = '\0';
        currentState = STATE_AUDIO_ENCODE;
    }

    // Description
    const char* desc = "Upload audio, generate random audio, or download from YouTube";
    DrawText(desc, screenWidth / 2 - MeasureText(desc, 16) / 2, 320, 16, GRAY);

    DrawStatusMessage();
}

void DrawAudioSteganography() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    // Title
    const char* title = "Audio Steganography";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 100, 30, DARKGRAY);

    // Back button
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) {
        currentState = STATE_MAIN_MENU;
    }

    // Options
    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Encode Message")) {
        currentState = STATE_AUDIO_OPTIONS;
        selectedAudioSource = AUDIO_NONE;
    }
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Decode Message")) {
        currentState = STATE_AUDIO_DECODE;
        filePathBuffer[0] = '\0';
        decodedMessageBuffer[0] = '\0';
    }

    // Description
    const char* desc = "Select an option to encode or decode messages in audio files";
    DrawText(desc, screenWidth / 2 - MeasureText(desc, 16) / 2, 400, 16, GRAY);

    DrawStatusMessage();
}

int main(void) {
    InitWindow(1000, 650, "StegaNet - Steganography Application");
    InitAudioDevice();
    SetTargetFPS(60);

    // Initialize random seed
    SetRandomSeed(time(NULL));

    while (!WindowShouldClose()) {
        UpdateStatusMessage();

        BeginDrawing();
        ClearBackground(RAYWHITE);

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
            case STATE_YOUTUBE_INPUT:
                // Currently unused or placeholder
                break;
        }

        EndDrawing();
    }

    // Cleanup
    if (encodedTextureLoaded) UnloadTexture(encodedTexture);
    if (audioWaveLoaded) UnloadWave(audioWave);
    if (audioSoundLoaded) UnloadSound(audioSound);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

