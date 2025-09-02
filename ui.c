#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "common.h"
#include "ui.h"
#include "utils.h"
#include "network.h"
#include "steganography.h"


void DrawConnectionDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 400;
    int dialogHeight = 300;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("StegaChat Connection", dialogX + 20, dialogY + 20, 20, DARKGRAY);
    
    // Server/Client toggle
    if (GuiButton((Rectangle){dialogX + 50, dialogY + 60, 120, 40}, isServer ? "Server Mode" : "Client Mode")) {
        isServer = !isServer;
    }
    
    if (!isServer) {
        DrawText("Server IP:", dialogX + 20, dialogY + 120, 16, DARKGRAY);
        if (GuiTextBox((Rectangle){dialogX + 100, dialogY + 115, 200, 30}, serverIPBuffer, 19, serverIPEditMode)) {
            serverIPEditMode = !serverIPEditMode;
        }
    }
    
    DrawText("Port:", dialogX + 20, dialogY + 160, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){dialogX + 100, dialogY + 155, 100, 30}, portBuffer, 9, portEditMode)) {
        portEditMode = !portEditMode;
    }
    
    // Connect button
    if (GuiButton((Rectangle){dialogX + 150, dialogY + 210, 100, 40}, "Connect")) {
        int port = atoi(portBuffer);
        if (port > 0 && port < 65536) {
            InitializeConnection(isServer, serverIPBuffer, port);
        } else {
            ShowStatus("Invalid port number");
        }
    }
    
    // Status
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        DrawText(statusMessage, dialogX + 20, dialogY + 260, 14, RED);
    }
}
void DrawDecodeDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 500;
    int dialogHeight = 400;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("Decode Hidden Message", dialogX + 20, dialogY + 20, 18, DARKGRAY);
    
    // If we're decoding from a specific message
    if (selectedMessageIndex >= 0 && selectedMessageIndex < messageCount) {
        ChatMessage* msg = &messages[selectedMessageIndex];
        
        // Show message info
        char msgInfo[256];
        sprintf(msgInfo, "Decoding from: %s", msg->content);
        DrawText(msgInfo, dialogX + 20, dialogY + 60, 14, DARKGRAY);
        
        // Show image if it's an image message
        if (msg->type == MSG_IMAGE && !msg->isSent) {
            char imagePath[512];
            sprintf(imagePath, "received_%s", strrchr(msg->content, ']') + 2);
            
            if (FileExists(imagePath)) {
                if (!imageLoaded) {
                    LoadImageForDisplay(imagePath);
                }
                
                if (imageLoaded) {
                    int imgX = dialogX + (dialogWidth - currentImageTexture.width) / 2;
                    int imgY = dialogY + 90;
                    DrawTexture(currentImageTexture, imgX, imgY, WHITE);
                }
            }
        }
        
        // Decode button for the selected message
        if (GuiButton((Rectangle){dialogX + 20, dialogY + 320, 100, 30}, "Decode This")) {
            char filePath[512];
            if (msg->type == MSG_IMAGE) {
                sprintf(filePath, "received_%s", strrchr(msg->content, ']') + 2);
                char* decoded = DecodeMessageFromImage(filePath);
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found");
                }
            } else if (msg->type == MSG_AUDIO) {
                sprintf(filePath, "received_%s", strrchr(msg->content, ']') + 2);
                char* decoded = DecodeMessageFromAudio(filePath);
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found");
                }
            }
        }
    } else {
        // Manual file decode
        DrawText("File path to decode:", dialogX + 20, dialogY + 60, 14, DARKGRAY);
        if (GuiTextBox((Rectangle){dialogX + 20, dialogY + 80, 350, 30}, decodeFilePath, 255, decodeFilePathEditMode)) {
            decodeFilePathEditMode = !decodeFilePathEditMode;
        }
        
        // Auto-detect file type and decode
        if (GuiButton((Rectangle){dialogX + 380, dialogY + 80, 80, 30}, "Decode")) {
            if (strlen(decodeFilePath) > 0 && FileExists(decodeFilePath)) {
                char* decoded = NULL;
                
                // Check file extension to determine type
                const char* ext = strrchr(decodeFilePath, '.');
                if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)) {
                    decoded = DecodeMessageFromImage(decodeFilePath);
                    if (!imageLoaded) {
                        LoadImageForDisplay(decodeFilePath);
                    }
                } else if (ext && strcmp(ext, ".wav") == 0) {
                    decoded = DecodeMessageFromAudio(decodeFilePath);
                }
                
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found or unsupported format");
                }
            } else {
                ShowStatus("File not found!");
            }
        }
        
        // Show loaded image
        if (imageLoaded) {
            int imgX = dialogX + (dialogWidth - currentImageTexture.width) / 2;
            int imgY = dialogY + 120;
            DrawTexture(currentImageTexture, imgX, imgY, WHITE);
        }
    }
    
    // Show decoded message
    if (strlen(decodedHiddenMessage) > 0) {
        DrawText("Decoded message:", dialogX + 20, dialogY + 280, 14, (Color){255, 152, 0, 255});
        DrawRectangle(dialogX + 20, dialogY + 300, dialogWidth - 40, 40, (Color){255, 248, 225, 255});
        DrawRectangleLines(dialogX + 20, dialogY + 300, dialogWidth - 40, 40, (Color){255, 193, 7, 255});
        DrawText(decodedHiddenMessage, dialogX + 25, dialogY + 315, 12, DARKGRAY);
    }
    
    // Close button
    if (GuiButton((Rectangle){dialogX + dialogWidth - 80, dialogY + 360, 60, 25}, "Close")) {
        showDecodeDialog = false;
        selectedMessageIndex = -1;
        decodedHiddenMessage[0] = '\0';
        decodeFilePath[0] = '\0';
        if (imageLoaded) {
            UnloadTexture(currentImageTexture);
            imageLoaded = false;
        }
    }
}

void DrawYTDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 500;
    int dialogHeight = 300;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("YouTube Audio Download", dialogX + 20, dialogY + 20, 18, DARKGRAY);
    
    // Instructions
    DrawText("Enter any YouTube URL format:", dialogX + 20, dialogY + 60, 14, GRAY);
    DrawText("(Full URLs, youtu.be links, video IDs, etc.)", dialogX + 20, dialogY + 80, 12, LIGHTGRAY);
    
    // URL input - now accepts full URLs
    if (GuiTextBox((Rectangle){dialogX + 20, dialogY + 105, 350, 30}, 
                   ytUrlBuffer, 511, ytUrlEditMode)) {
        ytUrlEditMode = !ytUrlEditMode;
    }
    
    // Download button
    const char* downloadButtonText = isDownloading ? "Downloading..." : "Download";
    Color downloadButtonColor = isDownloading ? LIGHTGRAY : (Color){76, 175, 80, 255};
    
    if (GuiButton((Rectangle){dialogX + 380, dialogY + 105, 90, 30}, downloadButtonText)) {
        if (!isDownloading && strlen(ytUrlBuffer) > 0) {
            // Pass the URL as-is - the script now handles all URL formats
            DownloadFromYoutube(ytUrlBuffer);
        }
    }
    
    // Instructions text
    DrawText("Downloaded audio will be automatically selected for encoding/sending.", 
             dialogX + 20, dialogY + 155, 12, GRAY);
    DrawText("Make sure yt-dlp is installed and yt_downloader.js is in the directory.", 
             dialogX + 20, dialogY + 175, 12, GRAY);
    
    // Status area
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        DrawRectangle(dialogX + 20, dialogY + 200, dialogWidth - 40, 50, (Color){248, 249, 250, 255});
        DrawRectangleLines(dialogX + 20, dialogY + 200, dialogWidth - 40, 50, LIGHTGRAY);
        DrawText("Status:", dialogX + 30, dialogY + 210, 12, DARKGRAY);
        
        // Word wrap for status message
        const char* status = statusMessage;
        int maxWidth = dialogWidth - 60;
        int lineY = dialogY + 230;
        
        // Simple text wrapping for status
        if (MeasureText(status, 12) > maxWidth) {
            char line1[128], line2[128];
            int splitPoint = strlen(status) / 2;
            // Find nearest space
            while (splitPoint > 0 && status[splitPoint] != ' ') splitPoint--;
            if (splitPoint == 0) splitPoint = strlen(status) / 2;
            
            strncpy(line1, status, splitPoint);
            line1[splitPoint] = '\0';
            strcpy(line2, status + splitPoint + 1);
            
            DrawText(line1, dialogX + 30, lineY, 12, (Color){76, 175, 80, 255});
            DrawText(line2, dialogX + 30, lineY + 15, 12, (Color){76, 175, 80, 255});
        } else {
            DrawText(status, dialogX + 30, lineY, 12, (Color){76, 175, 80, 255});
        }
    }
    
    // Close button
    if (GuiButton((Rectangle){dialogX + dialogWidth - 80, dialogY + 260, 60, 25}, "Close")) {
        showYTDialog = false;
        ytUrlBuffer[0] = '\0';
    }
}



void DrawChat() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Background gradient
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                           (Color){229, 221, 213, 255}, 
                           (Color){219, 209, 199, 255});
    
    // Header
    DrawRectangle(0, 0, screenWidth, 70, (Color){7, 94, 84, 255});
    
    // Contact info
    DrawCircle(50, 35, 20, WHITE);
    DrawText("C", 44, 27, 20, (Color){7, 94, 84, 255});
    DrawText(connection.isConnected ? "Contact" : "Not Connected", 80, 20, 18, WHITE);
    DrawText(connection.isConnected ? "Online" : "Offline", 80, 42, 14, (Color){200, 200, 200, 255});
    
    // Connection info
    if (connection.isConnected) {
        char connInfo[100];
        sprintf(connInfo, "Connected to: %s:%d", connection.remoteIP, connection.remotePort);
        int textWidth = MeasureText(connInfo, 12);
        DrawText(connInfo, screenWidth - textWidth - 10, 25, 12, (Color){200, 200, 200, 255});
    }
    
    // Disconnect button
    if (connection.isConnected) {
        if (GuiButton((Rectangle){screenWidth - 100, 40, 80, 25}, "Disconnect")) {
            CloseConnection();
            showConnectionDialog = true;
        }
    }
    
    // Messages area
    int chatAreaY = 70;
    int chatAreaHeight = screenHeight - 220; // More space for enhanced input area
    int messageY = chatAreaY + 10 - (int)scrollOffset;
    
    // Draw messages
    pthread_mutex_lock(&messageMutex);
    for (int i = 0; i < messageCount; i++) {
        ChatMessage* msg = &messages[i];
        
        int msgWidth = 350;
        int msgX = msg->isSent ? screenWidth - msgWidth - 20 : 20;
        
        // Calculate message height based on type
        int msgHeight = 60;
        if (msg->type == MSG_IMAGE || msg->type == MSG_AUDIO) {
            msgHeight = 120; // Larger for media messages
        }
        
        // Skip if outside visible area
        if (messageY + msgHeight < chatAreaY || messageY > chatAreaY + chatAreaHeight) {
            messageY += msgHeight + 10;
            continue;
        }
        
        // Message bubble
        Color bubbleColor = msg->isSent ? (Color){220, 248, 198, 255} : WHITE;
        DrawRectangleRounded((Rectangle){msgX, messageY, msgWidth, msgHeight}, 0.1f, 8, bubbleColor);
        DrawRectangleLinesEx((Rectangle){msgX, messageY, msgWidth, msgHeight}, 1, (Color){200, 200, 200, 255});
        
        // Draw content based on message type
        if (msg->type == MSG_TEXT) {
            // Text message - simple wrapping
            const char* text = msg->content;
            int textY = messageY + 10;
            int maxWidth = msgWidth - 20;
            
            // Simple text display
            DrawText(text, msgX + 10, textY, 14, DARKGRAY);
        } else if (msg->type == MSG_IMAGE) {
            // Image message
            DrawText("🖼️ Image:", msgX + 10, messageY + 10, 14, DARKGRAY);
            
            // Load and display the actual image (both sent and received)
            char imagePath[512];
            if (msg->isSent) {
                // For sent messages, try to find the original file
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                strcpy(imagePath, filename);
            } else {
                // For received messages
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                sprintf(imagePath, "received_%s", filename);
            }
            
            // Display image if available
            if (FileExists(imagePath)) {
                if (!messageImageLoaded[i]) {
                    Image img = LoadImage(imagePath);
                    if (img.data != NULL) {
                        // Resize for message display
                        float scale = fminf(150.0f / img.width, 80.0f / img.height);
                        int newWidth = (int)(img.width * scale);
                        int newHeight = (int)(img.height * scale);
                        ImageResize(&img, newWidth, newHeight);
                        
                        messageImages[i] = LoadTextureFromImage(img);
                        UnloadImage(img);
                        messageImageLoaded[i] = true;
                    }
                }
                
                if (messageImageLoaded[i]) {
                    DrawTexture(messageImages[i], msgX + 10, messageY + 25, WHITE);
                    
                    // Click to view full image
                    Rectangle imgRect = {msgX + 10, messageY + 25, messageImages[i].width, messageImages[i].height};
                    if (CheckCollisionPointRec(GetMousePosition(), imgRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedMessageIndex = i;
                        LoadImageForDisplay(imagePath);
                        showDecodeDialog = true;
                    }
                }
            } else {
                DrawText(msg->content, msgX + 10, messageY + 25, 12, GRAY);
            }
            
            // Decode button for all images (both sent and received)
            if (GuiButton((Rectangle){msgX + msgWidth - 80, messageY + msgHeight - 35, 70, 25}, "Decode")) {
                selectedMessageIndex = i;
                showDecodeDialog = true;
            }
        } else if (msg->type == MSG_AUDIO) {
            // Audio message
            DrawText("🎵 Audio:", msgX + 10, messageY + 10, 14, DARKGRAY);
            
            char audioPath[512];
            if (msg->isSent) {
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                strcpy(audioPath, filename);
            } else {
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                sprintf(audioPath, "received_%s", filename);
            }
            
            DrawText(msg->content, msgX + 10, messageY + 25, 12, GRAY);
            
            // Load audio if not already loaded
            if (!messageAudioLoaded[i] && FileExists(audioPath)) {
                messageAudioSounds[i] = LoadSound(audioPath);
                if (IsSoundValid(messageAudioSounds[i])) {
                    messageAudioLoaded[i] = true;
                }
            }
            
            // Audio controls
            if (messageAudioLoaded[i]) {
                // Play/Stop button
                const char* playButtonText = audioPlaying[i] ? "Stop" : "Play";
                if (GuiButton((Rectangle){msgX + 10, messageY + 45, 50, 25}, playButtonText)) {
                    if (audioPlaying[i]) {
                        StopSound(messageAudioSounds[i]);
                        audioPlaying[i] = false;
                    } else {
                        // Stop all other audio first
                        for (int j = 0; j < MAX_MESSAGES; j++) {
                            if (audioPlaying[j] && messageAudioLoaded[j]) {
                                StopSound(messageAudioSounds[j]);
                                audioPlaying[j] = false;
                            }
                        }
                        PlaySound(messageAudioSounds[i]);
                        audioPlaying[i] = true;
                    }
                }
                
                // Check if sound finished playing
                if (audioPlaying[i] && !IsSoundPlaying(messageAudioSounds[i])) {
                    audioPlaying[i] = false;
                }
            } else {
                DrawRectangle(msgX + 10, messageY + 45, 100, 30, (Color){173, 216, 230, 255});
                DrawText("♪ Audio File", msgX + 15, messageY + 55, 12, DARKBLUE);
            }
            
            // Decode button for all audio files
            if (GuiButton((Rectangle){msgX + msgWidth - 80, messageY + msgHeight - 35, 70, 25}, "Decode")) {
                selectedMessageIndex = i;
                showDecodeDialog = true;
            }
        }
        
        // Hidden message indicator
        if (msg->hasHiddenMessage) {
            DrawRectangle(msgX + 5, messageY + msgHeight - 20, 15, 15, (Color){255, 193, 7, 255});
            DrawText("🔒", msgX + 8, messageY + msgHeight - 18, 10, DARKGRAY);
        }
        
        // Timestamp
        DrawText(msg->timestamp, msgX + msgWidth - 60, messageY + msgHeight - 20, 10, GRAY);
        
        messageY += msgHeight + 10;
    }
    pthread_mutex_unlock(&messageMutex);
    
    // Handle scrolling
    float wheel = GetMouseWheelMove();
    scrollOffset -= wheel * 40;
    if (scrollOffset < 0) scrollOffset = 0;
    
    // Enhanced input area
    int inputAreaY = screenHeight - 150;
    DrawRectangle(0, inputAreaY, screenWidth, 150, (Color){240, 240, 240, 255});
    DrawLine(0, inputAreaY, screenWidth, inputAreaY, LIGHTGRAY);
    
    // Tool buttons row
    int buttonY = inputAreaY + 10;
    int buttonWidth = 70;
    int buttonSpacing = 80;
    int startX = 20;
    
    // Generate Random Image button
    if (GuiButton((Rectangle){startX, buttonY, buttonWidth, 25}, "Gen Image")) {
        GenerateRandomImage();
    }
    
    // Generate Random Audio button
    if (GuiButton((Rectangle){startX + buttonSpacing, buttonY, buttonWidth, 25}, "Gen Audio")) {
        GenerateRandomAudio();
    }
    
    // YouTube Download button
    if (GuiButton((Rectangle){startX + buttonSpacing * 2, buttonY, buttonWidth, 25}, "YT Download")) {
        showYTDialog = true;
    }
    
    // Encode button
    if (GuiButton((Rectangle){startX + buttonSpacing * 3, buttonY, buttonWidth, 25}, "Encode")) {
        if (strlen(selectedFilePath) > 0 && strlen(hiddenMessageBuffer) > 0) {
            char outputPath[512];
            sprintf(outputPath, "encoded_%d_%s", (int)time(NULL), 
                   strrchr(selectedFilePath, '/') ? strrchr(selectedFilePath, '/') + 1 : selectedFilePath);
            
            if (selectedMessageType == MSG_IMAGE) {
                EncodeMessageInImage(selectedFilePath, hiddenMessageBuffer, outputPath);
                strcpy(selectedFilePath, outputPath);
                ShowStatus("Image encoded! Ready to send.");
            } else if (selectedMessageType == MSG_AUDIO) {
                EncodeMessageInAudio(selectedFilePath, hiddenMessageBuffer, outputPath);
                strcpy(selectedFilePath, outputPath);
                ShowStatus("Audio encoded! Ready to send.");
            }
        } else {
            ShowStatus("Need file path and hidden message to encode");
        }
    }
    
    // Send Encoded button (separate from regular send)
    if (GuiButton((Rectangle){startX + buttonSpacing * 4, buttonY, buttonWidth, 25}, "Send Encoded")) {
        if (connection.isConnected && strlen(selectedFilePath) > 0 && FileExists(selectedFilePath)) {
            if (selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) {
                SendFile(selectedFilePath, selectedMessageType);
                selectedFilePath[0] = '\0';
                hiddenMessageBuffer[0] = '\0';
                ShowStatus("Encoded file sent!");
            }
        } else if (!connection.isConnected) {
            ShowStatus("Not connected!");
        } else {
            ShowStatus("No encoded file to send!");
        }
    }
    
    // Decode button
    if (GuiButton((Rectangle){startX + buttonSpacing * 5, buttonY, buttonWidth, 25}, "Decode File")) {
        showDecodeDialog = true;
        selectedMessageIndex = -1; // For manual decode
    }
    
    // Message type selector
    buttonY += 35;
    DrawText("Type:", startX, buttonY + 5, 14, DARKGRAY);
    
    if (GuiButton((Rectangle){startX + 50, buttonY, 60, 25}, selectedMessageType == MSG_TEXT ? "TEXT" : "Text")) {
        selectedMessageType = MSG_TEXT;
    }
    if (GuiButton((Rectangle){startX + 120, buttonY, 60, 25}, selectedMessageType == MSG_IMAGE ? "IMAGE" : "Image")) {
        selectedMessageType = MSG_IMAGE;
    }
    if (GuiButton((Rectangle){startX + 190, buttonY, 60, 25}, selectedMessageType == MSG_AUDIO ? "AUDIO" : "Audio")) {
        selectedMessageType = MSG_AUDIO;
    }
    
    // File path input for media
    if (selectedMessageType != MSG_TEXT) {
        buttonY += 35;
        DrawText("File:", startX, buttonY + 5, 12, DARKGRAY);
        if (GuiTextBox((Rectangle){startX + 40, buttonY, 400, 25}, selectedFilePath, 255, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
        
        // Browse button
        if (GuiButton((Rectangle){startX + 450, buttonY, 60, 25}, "Browse")) {
            ShowStatus("Use file picker or type path manually");
        }
    }
    
    // Hidden message input (for steganography)
    if (selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) {
        buttonY += 35;
        DrawText("Hidden:", startX, buttonY + 5, 12, GRAY);
        if (GuiTextBox((Rectangle){startX + 60, buttonY, screenWidth - 200, 25}, hiddenMessageBuffer, MAX_MESSAGE_LENGTH - 1, hiddenMessageEditMode)) {
            hiddenMessageEditMode = !hiddenMessageEditMode;
        }
    }
    
    // Main message input
    buttonY += 35;
    DrawText("Message:", startX, buttonY + 5, 14, DARKGRAY);
    if (GuiTextBox((Rectangle){startX + 70, buttonY, screenWidth - 180, 25}, inputBuffer, MAX_MESSAGE_LENGTH - 1, inputEditMode)) {
        inputEditMode = !inputEditMode;
    }
    
    // Send button
    bool canSend = false;
    if (selectedMessageType == MSG_TEXT && strlen(inputBuffer) > 0) {
        canSend = true;
    } else if ((selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) && 
               strlen(selectedFilePath) > 0 && FileExists(selectedFilePath)) {
        canSend = true;
    }
    
    Color sendButtonColor = canSend ? (Color){76, 175, 80, 255} : LIGHTGRAY;
    if (GuiButton((Rectangle){screenWidth - 90, buttonY, 70, 25}, "Send")) {
        if (canSend && connection.isConnected) {
            if (selectedMessageType == MSG_TEXT) {
                SendMessage(inputBuffer, MSG_TEXT);
                inputBuffer[0] = '\0';
            } else {
                SendFile(selectedFilePath, selectedMessageType);
                selectedFilePath[0] = '\0';
                hiddenMessageBuffer[0] = '\0'; // Clear hidden message after sending
            }
        } else if (!connection.isConnected) {
            ShowStatus("Not connected!");
        }
    }
    
    // Status message
    if (statusTimer > 0) {
        int statusX = screenWidth / 2 - MeasureText(statusMessage, 14) / 2;
        DrawText(statusMessage, statusX, inputAreaY - 25, 14, (Color){76, 175, 80, 255});
    }
    
    // Decode dialog
    if (showDecodeDialog) {
        DrawDecodeDialog();
    }
    
    // YouTube download dialog
    if (showYTDialog) {
        DrawYTDialog();
    }
}
