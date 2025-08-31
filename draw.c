#include "globals.h"
#include "utils.h"
#include "image_stego.h"
#include "audio_stego.h"
#include "youtube.h"
#include "raygui.h"
#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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