#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>


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
bool ytLinkEditMode = false;
bool messageEditMode = false;
Texture2D encodedTexture;
bool encodedTextureLoaded = false;


void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating random image...");
    system("gcc -I$(brew --prefix libpng)/include -L$(brew --prefix libpng)/lib -o imageGen imageGen.c -lpng && ./imageGen");
}

void GenerateRandomAudio() {
    TraceLog(LOG_INFO, "Generating random audio...");
    
}

void DownloadFromYouTube(const char* url) {
    size_t command_buffer_size = strlen("python3 downloadAudio.py ") + strlen(url) + 10;
    char* command = (char*)malloc(command_buffer_size);
    snprintf(command, command_buffer_size, "python3 downloadAudio.py %s", url);
    int result = system(command);
    free(command);
    command = NULL;
}

void DrawImageEncode() {
    
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
        if (encodedTextureLoaded) {
            UnloadTexture(encodedTexture);
            encodedTextureLoaded = false;
        }
        currentState = STATE_IMAGE_OPTIONS;
        return;
    }

    
    const char* title = "Encode Message in Image";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    
    char sourceText[100];
    switch (selectedImageSource) {
        case IMAGE_UPLOAD: strcpy(sourceText, "Source: Uploaded Image"); break;
        case IMAGE_GENERATE: strcpy(sourceText, "Source: Generated Image"); break;
        default: strcpy(sourceText, "Source: None"); break;
    }
    DrawText(sourceText, screenWidth / 2 - MeasureText(sourceText, 18) / 2, 100, 18, DARKGRAY);

    
    DrawText("Enter message to hide:", screenWidth / 2 - 200, 140, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 170, 400, 30 }, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }

    
    if (!encodedTextureLoaded) {
        if (FileExists("randomImage.png")) {
            encodedTexture = LoadTexture("randomImage.png");
            encodedTextureLoaded = true;
        } else {
            DrawText("Failed to load image!", screenWidth / 2 - 100, 250, 20, RED);
            return;
        }
    }

    
    float imageY = 220;
    DrawTextureEx(encodedTexture, (Vector2){ screenWidth / 2 - encodedTexture.width / 2.0f, imageY }, 0, 1.0f, WHITE);

    
    float buttonY = imageY + encodedTexture.height + 20;
    if (GuiButton((Rectangle){ screenWidth / 2 - 160, buttonY, 140, 40 }, "Encode")) {
        TraceLog(LOG_INFO, "Encoding message: %s", messageBuffer);
        
    }

    if (GuiButton((Rectangle){ screenWidth / 2 + 20, buttonY, 140, 40 }, "Save Result")) {
        TraceLog(LOG_INFO, "Saving encoded image...");
        
    }

    
    const char* tip = "Click on the text box to edit the message";
    DrawText(tip, screenWidth / 2 - MeasureText(tip, 14) / 2, buttonY + 50, 14, GRAY);
}


void DrawImageDecode() {
    ClearBackground(RAYWHITE);
    DrawText("DrawImageDecode() not implemented yet.", 300, 300, 20, DARKGRAY);
}

void DrawAudioEncode() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();

    
    if (GuiButton((Rectangle){ 30, 30, 100, 40 }, "Back")) {
        if (encodedTextureLoaded) {
            UnloadTexture(encodedTexture);
            encodedTextureLoaded = false;
        }
        currentState = STATE_AUDIO_OPTIONS;
        return;
    }

    
    const char* title = "Encode Message in Audio";
    DrawText(title, screenWidth / 2 - MeasureText(title, 30) / 2, 50, 30, DARKGRAY);

    
    char sourceText[100];
    switch (selectedImageSource) {
        case AUDIO_UPLOAD: strcpy(sourceText, "Source: Uploaded Audio"); break;
        case AUDIO_GENERATE: strcpy(sourceText, "Source: Generated Audio"); break;
        default: strcpy(sourceText, "Source: None"); break;
    }
    DrawText(sourceText, screenWidth / 2 - MeasureText(sourceText, 18) / 2, 100, 18, DARKGRAY);

    
    DrawText("Enter message to hide:", screenWidth / 2 - 200, 140, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ screenWidth / 2 - 200, 170, 400, 30 }, messageBuffer, 512, messageEditMode)) {
        messageEditMode = !messageEditMode;
    }

    
    if (!encodedTextureLoaded) {
        if (FileExists("randomImage.png")) {
            encodedTexture = LoadTexture("randomImage.png");
            encodedTextureLoaded = true;
        } else {
            DrawText("Failed to load image!", screenWidth / 2 - 100, 250, 20, RED);
            return;
        }
    }

    
    float imageY = 220;
    DrawTextureEx(encodedTexture, (Vector2){ screenWidth / 2 - encodedTexture.width / 2.0f, imageY }, 0, 1.0f, WHITE);

    
    float buttonY = imageY + encodedTexture.height + 20;
    if (GuiButton((Rectangle){ screenWidth / 2 - 160, buttonY, 140, 40 }, "Encode")) {
        TraceLog(LOG_INFO, "Encoding message: %s", messageBuffer);
        
    }

    if (GuiButton((Rectangle){ screenWidth / 2 + 20, buttonY, 140, 40 }, "Save Result")) {
        TraceLog(LOG_INFO, "Saving encoded image...");
        
    }

    
    const char* tip = "Click on the text box to edit the message";
    DrawText(tip, screenWidth / 2 - MeasureText(tip, 14) / 2, buttonY + 50, 14, GRAY);
}

void DrawAudioDecode() {
    ClearBackground(RAYWHITE);
    DrawText("DrawAudioDecode() not implemented yet.", 300, 300, 20, DARKGRAY);
}

void DrawMainMenu() {
    ClearBackground(RAYWHITE);
    DrawText("StegaNet", 400, 100, 40, DARKGRAY);
    DrawText("Choose Steganography Type", 350, 160, 20, GRAY);

    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Audio Steganography")) currentState = STATE_AUDIO_STEGO;
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Image Steganography")) currentState = STATE_IMAGE_STEGO;
    if (GuiButton((Rectangle){ 450, 400, 100, 50 }, "Exit")) TraceLog(LOG_INFO, "Exit Button Clicked!");
}

void DrawImageSteganography() {
    ClearBackground(RAYWHITE);
    DrawText("Image Steganography", 350, 100, 30, DARKGRAY);
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) currentState = STATE_MAIN_MENU;
    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Encode Message")) { currentState = STATE_IMAGE_OPTIONS; selectedImageSource = IMAGE_NONE; }
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Decode Message")) currentState = STATE_IMAGE_DECODE;
    DrawText("Select an option to encode or decode messages in images", 200, 400, 16, GRAY);
}

void DrawAudioSteganography() {
    ClearBackground(RAYWHITE);
    DrawText("Audio Steganography", 350, 100, 30, DARKGRAY);
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) currentState = STATE_MAIN_MENU;
    if (GuiButton((Rectangle){ 170, 250, 250, 100 }, "Encode Message")) { currentState = STATE_AUDIO_OPTIONS; selectedAudioSource = AUDIO_NONE; }
    if (GuiButton((Rectangle){ 570, 250, 250, 100 }, "Decode Message")) currentState = STATE_AUDIO_DECODE;
    DrawText("Select an option to encode or decode messages in audio files", 200, 400, 16, GRAY);
}

void DrawImageOptions() {
    ClearBackground(RAYWHITE);
    DrawText("Image Source Options", 350, 100, 30, DARKGRAY);
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) currentState = STATE_IMAGE_STEGO;
    DrawText("Choose image source:", 200, 180, 20, DARKGRAY);

    if (GuiButton((Rectangle){ 150, 220, 200, 60 }, "Upload Image")) {
        selectedImageSource = IMAGE_UPLOAD;
        currentState = STATE_IMAGE_ENCODE;
    }
    if (GuiButton((Rectangle){ 400, 220, 200, 60 }, "Generate Random")) {
        selectedImageSource = IMAGE_GENERATE;
        GenerateRandomImage();
        currentState = STATE_IMAGE_ENCODE;
    }
    DrawText("Upload an existing image or generate a random one", 250, 320, 16, GRAY);
}

void DrawAudioOptions() {
    ClearBackground(RAYWHITE);
    DrawText("Audio Source Options", 350, 100, 30, DARKGRAY);
    if (GuiButton((Rectangle){ 30, 50, 100, 40 }, "Back")) currentState = STATE_AUDIO_STEGO;
    DrawText("Choose audio source:", 200, 180, 20, DARKGRAY);

    if (GuiButton((Rectangle){ 100, 220, 180, 60 }, "Upload Audio")) { selectedAudioSource = AUDIO_UPLOAD; currentState = STATE_AUDIO_ENCODE; }
    if (GuiButton((Rectangle){ 300, 220, 180, 60 }, "Generate Audio")) { selectedAudioSource = AUDIO_GENERATE; GenerateRandomAudio(); currentState = STATE_AUDIO_ENCODE; }
    if (GuiButton((Rectangle){ 500, 220, 180, 60 }, "YouTube Link")) { selectedAudioSource = AUDIO_YOUTUBE; currentState = STATE_AUDIO_ENCODE; }

    DrawText("Upload audio, generate random audio, or download from YouTube", 200, 320, 16, GRAY);
}


int main(void) {
    InitWindow(1000, 650, "StegaNet");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
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
        }

        EndDrawing();
    }

    if (encodedTextureLoaded) {
        UnloadTexture(encodedTexture);
    }

    CloseWindow();
    return 0;
}
