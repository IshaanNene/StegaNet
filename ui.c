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

static float dialogAnimTime = 0.0f;
static float pulseTime = 0.0f;
static float hoverTime[20] = {0}; 
static int hoveredButton = -1;

#define MODERN_DARK      (Color){26, 32, 44, 255}
#define MODERN_ACCENT    (Color){99, 102, 241, 255}
#define MODERN_SUCCESS   (Color){16, 185, 129, 255}
#define MODERN_WARNING   (Color){245, 158, 11, 255}
#define MODERN_ERROR     (Color){239, 68, 68, 255}
#define MODERN_SURFACE   (Color){255, 255, 255, 255}
#define MODERN_SURFACE_2 (Color){248, 250, 252, 255}
#define MODERN_BORDER    (Color){226, 232, 240, 255}
#define MODERN_TEXT      (Color){51, 65, 85, 255}
#define MODERN_TEXT_LIGHT (Color){100, 116, 139, 255}

void DrawGlassMorphRect(Rectangle rect, Color color, float blur) {
    DrawRectangleRounded(rect, 0.12f, 16, (Color){color.r, color.g, color.b, (unsigned char)(color.a * 0.1f)});
    DrawRectangleRounded(rect, 0.12f, 16, (Color){255, 255, 255, 25});
    DrawRectangleRoundedLines(rect, 0.12f, 16, (Color){255, 255, 255, 50});
}

bool DrawEnhancedButton(Rectangle rect, const char* text, Color normalColor, Color hoverColor, int buttonId) {
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool clicked = false;
    
    if (isHovered) {
        hoveredButton = buttonId;
        hoverTime[buttonId] = fminf(hoverTime[buttonId] + GetFrameTime() * 4.0f, 1.0f);
    } else {
        hoverTime[buttonId] = fmaxf(hoverTime[buttonId] - GetFrameTime() * 4.0f, 0.0f);
    }
    
    Color currentColor = {
        (unsigned char)(normalColor.r + (hoverColor.r - normalColor.r) * hoverTime[buttonId]),
        (unsigned char)(normalColor.g + (hoverColor.g - normalColor.g) * hoverTime[buttonId]),
        (unsigned char)(normalColor.b + (hoverColor.b - normalColor.b) * hoverTime[buttonId]),
        255
    };
    
    Rectangle shadowRect = {rect.x + 2, rect.y + 4, rect.width, rect.height};
    DrawRectangleRounded(shadowRect, 0.08f, 12, (Color){0, 0, 0, 20});
    
    float scale = 1.0f + hoverTime[buttonId] * 0.02f;
    Rectangle scaledRect = {
        rect.x - (rect.width * scale - rect.width) / 2,
        rect.y - (rect.height * scale - rect.height) / 2,
        rect.width * scale,
        rect.height * scale
    };
    
    DrawRectangleRounded(scaledRect, 0.08f, 12, currentColor);
    DrawRectangleRoundedLines(scaledRect, 0.0812, 1, (Color){255, 255, 255, (unsigned char)(100 * hoverTime[buttonId])});
    
    int textWidth = MeasureText(text, 14);
    int textX = rect.x + (rect.width - textWidth) / 2;
    int textY = rect.y + (rect.height - 14) / 2;
    
    DrawText(text, textX + 1, textY + 1, 14, (Color){0, 0, 0, 30}); 
    DrawText(text, textX, textY, 14, WHITE);
    
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        clicked = true;
    }
    
    return clicked;
}

void DrawConnectionDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    dialogAnimTime += GetFrameTime();
    float animScale = fminf(dialogAnimTime * 4.0f, 1.0f);
    animScale = 1.0f - powf(1.0f - animScale, 3.0f); 
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 120});
    
    int dialogWidth = 450;
    int dialogHeight = 380;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    Rectangle dialogRect = {
        dialogX + dialogWidth * (1 - animScale) / 2,
        dialogY + dialogHeight * (1 - animScale) / 2,
        dialogWidth * animScale,
        dialogHeight * animScale
    };
    
    DrawGlassMorphRect(dialogRect, MODERN_SURFACE, 10.0f);
    
    Rectangle headerRect = {dialogRect.x, dialogRect.y, dialogRect.width, 60};
    DrawRectangleRounded(headerRect, 0.12f, 16, MODERN_ACCENT);
    DrawRectangleRounded((Rectangle){headerRect.x, headerRect.y + 30, headerRect.width, 30}, 0.0f, 0, 
                        (Color){MODERN_ACCENT.r, MODERN_ACCENT.g, MODERN_ACCENT.b, 150});
    
    DrawText("StegaChat", dialogRect.x + 30, dialogRect.y + 20, 24, WHITE);
    DrawText("Secure Connection Setup", dialogRect.x + 30, dialogRect.y + 45, 12, (Color){255, 255, 255, 180});
    
    DrawCircle(dialogRect.x + dialogRect.width - 40, dialogRect.y + 30, 15, (Color){255, 255, 255, 50});
    
    int toggleY = dialogRect.y + 90;
    Rectangle toggleBg = {dialogRect.x + 50, toggleY, 140, 35};
    Color toggleColor = isServer ? MODERN_SUCCESS : MODERN_TEXT_LIGHT;
    
    DrawRectangleRounded(toggleBg, 0.5f, 16, (Color){toggleColor.r, toggleColor.g, toggleColor.b, 30});
    DrawRectangleRoundedLines(toggleBg, 0.5f,  2, toggleColor);
    float toggleX = isServer ? toggleBg.x + toggleBg.width - 32 : toggleBg.x + 5;
    DrawCircle(toggleX + 12, toggleBg.y + toggleBg.height/2, 12, toggleColor);
    DrawCircle(toggleX + 12, toggleBg.y + toggleBg.height/2, 10, WHITE);
    
    DrawText(isServer ? "Server Mode" : "Client Mode", dialogRect.x + 210, toggleY + 10, 14, MODERN_TEXT);
    
    if (CheckCollisionPointRec(GetMousePosition(), toggleBg) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isServer = !isServer;
    }
    
    if (!isServer) {
        DrawText("Server Address", dialogRect.x + 50, dialogRect.y + 150, 12, MODERN_TEXT_LIGHT);
        Rectangle ipRect = {dialogRect.x + 50, dialogRect.y + 170, 250, 40};
        DrawRectangleRounded(ipRect, 0.06f, 8, MODERN_SURFACE_2);
        DrawRectangleRoundedLines(ipRect, 0.06f, 8, serverIPEditMode ? MODERN_ACCENT : MODERN_BORDER);
        
        if (GuiTextBox(ipRect, serverIPBuffer, 19, serverIPEditMode)) {
            serverIPEditMode = !serverIPEditMode;
        }
        
        bool validIP = strlen(serverIPBuffer) > 6; 
        DrawCircle(dialogRect.x + 315, dialogRect.y + 190, 6, validIP ? MODERN_SUCCESS : MODERN_ERROR);
    }
    
    DrawText("Port Number", dialogRect.x + 50, dialogRect.y + 230, 12, MODERN_TEXT_LIGHT);
    Rectangle portRect = {dialogRect.x + 50, dialogRect.y + 250, 120, 40};
    DrawRectangleRounded(portRect, 0.06f, 8, MODERN_SURFACE_2);
    DrawRectangleRoundedLines(portRect, 0.06f, 1, portEditMode ? MODERN_ACCENT : MODERN_BORDER);
    
    if (GuiTextBox(portRect, portBuffer, 9, portEditMode)) {
        portEditMode = !portEditMode;
    }
    
    int port = atoi(portBuffer);
    bool validPort = port > 0 && port < 65536;
    DrawCircle(dialogRect.x + 185, dialogRect.y + 270, 6, validPort ? MODERN_SUCCESS : MODERN_ERROR);

    pulseTime += GetFrameTime();
    float pulse = (sinf(pulseTime * 3.0f) + 1.0f) / 2.0f;
    Rectangle connectRect = {dialogRect.x + 180, dialogRect.y + 310, 120, 45};
    
    Color connectColor = validPort ? MODERN_SUCCESS : MODERN_TEXT_LIGHT;
    if (validPort) {
        DrawCircleLines(connectRect.x + connectRect.width/2, connectRect.y + connectRect.height/2, 
                       30 + pulse * 5, (Color){connectColor.r, connectColor.g, connectColor.b, (unsigned char)(50 * pulse)});
    }
    
    if (DrawEnhancedButton(connectRect, "Connect", connectColor, 
                          (Color){connectColor.r - 20, connectColor.g - 20, connectColor.b - 20, 255}, 0)) {
        if (validPort) {
            InitializeConnection(isServer, serverIPBuffer, port);
        } else {
            ShowStatus("Invalid port number");
        }
    }
    
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        Rectangle statusRect = {dialogRect.x + 20, dialogRect.y + dialogRect.height - 40, dialogRect.width - 40, 25};
        Color statusColor = strstr(statusMessage, "Error") ? MODERN_ERROR : MODERN_WARNING;
        
        DrawRectangleRounded(statusRect, 0.06f, 8, (Color){statusColor.r, statusColor.g, statusColor.b, 20});
        DrawRectangleRoundedLines(statusRect, 0.068, 1, statusColor);
        DrawText(statusMessage, statusRect.x + 10, statusRect.y + 6, 12, statusColor);
    }
}

void DrawDecodeDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 120});
    int dialogWidth = 600;
    int dialogHeight = 500;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    Rectangle dialogRect = {dialogX, dialogY, dialogWidth, dialogHeight};
    DrawRectangleRounded(dialogRect, 0.08f, 12, MODERN_SURFACE);
    DrawRectangleRoundedLines(dialogRect, 0.08f, 12, MODERN_BORDER);
    Rectangle headerRect = {dialogRect.x, dialogRect.y, dialogRect.width, 60};
    DrawRectangleRounded(headerRect, 0.08f, 12, MODERN_WARNING);
    
    DrawText("Decode Hidden Message", dialogRect.x + 20, dialogRect.y + 20, 18, WHITE);
    DrawText("Extract secret data from media files", dialogRect.x + 20, dialogRect.y + 40, 12, (Color){255, 255, 255, 180});
    
    int contentY = dialogRect.y + 80;
    
    if (selectedMessageIndex >= 0 && selectedMessageIndex < messageCount) {
        ChatMessage* msg = &messages[selectedMessageIndex];
        DrawText("Decoding from selected message:", dialogRect.x + 20, contentY, 14, MODERN_TEXT);
        
        char msgInfo[256];
        snprintf(msgInfo, sizeof(msgInfo), "%.50s%s", msg->content, strlen(msg->content) > 50 ? "..." : "");
        DrawText(msgInfo, dialogRect.x + 20, contentY + 25, 12, MODERN_TEXT_LIGHT);
        
        contentY += 60;
        
        if (msg->type == MSG_IMAGE && !msg->isSent) {
            char imagePath[512];
            sprintf(imagePath, "received_%s", strrchr(msg->content, ']') + 2);
            
            if (FileExists(imagePath)) {
                if (!imageLoaded) {
                    Image img = LoadImage(imagePath);
                    if (img.data != NULL) {
                        float maxWidth = 300.0f;
                        float maxHeight = 200.0f;
                        float scale = fminf(maxWidth / img.width, maxHeight / img.height);
                        
                        if (scale < 1.0f) {
                            ImageResize(&img, (int)(img.width * scale), (int)(img.height * scale));
                        }
                        
                        currentImageTexture = LoadTextureFromImage(img);
                        UnloadImage(img);
                        imageLoaded = true;
                    }
                }
                
                if (imageLoaded) {
                    int imgX = dialogRect.x + (dialogRect.width - currentImageTexture.width) / 2;
                    int imgY = contentY;
                    
                    DrawTexture(currentImageTexture, imgX, imgY, WHITE);
                    DrawRectangleLinesEx((Rectangle){imgX, imgY, currentImageTexture.width, currentImageTexture.height}, 
                                       1, MODERN_BORDER);
                    
                    contentY += currentImageTexture.height + 20;
                }
            }
        }
        
        if (DrawEnhancedButton((Rectangle){dialogRect.x + 30, contentY, 120, 35}, "Decode Message", MODERN_WARNING, MODERN_ERROR, 1)) {
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
        DrawText("File path to decode:", dialogRect.x + 20, contentY, 14, MODERN_TEXT);
        
        Rectangle fileRect = {dialogRect.x + 20, contentY + 25, 400, 30};
        DrawRectangleRounded(fileRect, 0.04f, 8, MODERN_SURFACE_2);
        DrawRectangleRoundedLines(fileRect, 0.04f, 8, decodeFilePathEditMode ? MODERN_ACCENT : MODERN_BORDER);
        
        if (GuiTextBox(fileRect, decodeFilePath, 255, decodeFilePathEditMode)) {
            decodeFilePathEditMode = !decodeFilePathEditMode;
        }
        
        if (DrawEnhancedButton((Rectangle){dialogRect.x + 440, contentY + 25, 80, 30}, "Decode", MODERN_WARNING, MODERN_ERROR, 3)) {
            if (strlen(decodeFilePath) > 0 && FileExists(decodeFilePath)) {
                char* decoded = NULL;
                
                const char* ext = strrchr(decodeFilePath, '.');
                if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)) {
                    decoded = DecodeMessageFromImage(decodeFilePath);
                    if (!imageLoaded) {
                        Image img = LoadImage(decodeFilePath);
                        if (img.data != NULL) {
                            float maxWidth = 300.0f;
                            float maxHeight = 200.0f;
                            float scale = fminf(maxWidth / img.width, maxHeight / img.height);
                            
                            if (scale < 1.0f) {
                                ImageResize(&img, (int)(img.width * scale), (int)(img.height * scale));
                            }
                            
                            currentImageTexture = LoadTextureFromImage(img);
                            UnloadImage(img);
                            imageLoaded = true;
                        }
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
        
        contentY += 80;
        
        if (imageLoaded) {
            int imgX = dialogRect.x + (dialogRect.width - currentImageTexture.width) / 2;
            int imgY = contentY;
            
            DrawTexture(currentImageTexture, imgX, imgY, WHITE);
            DrawRectangleLinesEx((Rectangle){imgX, imgY, currentImageTexture.width, currentImageTexture.height}, 
                               1, MODERN_BORDER);
            contentY += currentImageTexture.height + 20;
        }
    }
    if (strlen(decodedHiddenMessage) > 0) {
        Rectangle resultRect = {dialogRect.x + 20, dialogRect.y + dialogRect.height - 120, dialogRect.width - 40, 60};
        Color resultColor = strstr(decodedHiddenMessage, "No hidden") ? MODERN_ERROR : MODERN_SUCCESS;
        
        DrawRectangleRounded(resultRect, 0.06f, 8, (Color){resultColor.r, resultColor.g, resultColor.b, 30});
        DrawRectangleRoundedLines(resultRect, 0.06f, 8, resultColor);
        
        DrawText("Decoded Message:", resultRect.x + 10, resultRect.y + 10, 12, resultColor);
        DrawText(decodedHiddenMessage, resultRect.x + 10, resultRect.y + 30, 12, MODERN_TEXT);
    }
    Rectangle closeRect = {dialogRect.x + dialogRect.width - 80, dialogRect.y + dialogRect.height - 40, 60, 30};
    if (DrawEnhancedButton(closeRect, "Close", MODERN_TEXT_LIGHT, MODERN_ERROR, 4)) {
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
    
    dialogAnimTime += GetFrameTime();
    float animScale = fminf(dialogAnimTime * 4.0f, 1.0f);
    
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 110});
    
    int dialogWidth = 550;
    int dialogHeight = 350;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    Rectangle dialogRect = {dialogX, dialogY, dialogWidth * animScale, dialogHeight * animScale};
    
    DrawGlassMorphRect(dialogRect, MODERN_SURFACE, 12.0f);
    Rectangle headerRect = {dialogRect.x, dialogRect.y, dialogRect.width, 70};
    DrawRectangleRounded(headerRect, 0.12f, 16, MODERN_ERROR);
    
    DrawText("YouTube Downloader", dialogRect.x + 30, dialogRect.y + 20, 20, WHITE);
    DrawText("Download audio from any YouTube video", dialogRect.x + 30, dialogRect.y + 45, 12, (Color){255, 255, 255, 180});
    
    DrawRectangleRounded((Rectangle){dialogRect.x + dialogRect.width - 60, dialogRect.y + 15, 40, 25}, 0.2f, 8, WHITE);
    DrawText("YT", dialogRect.x + dialogRect.width - 52, dialogRect.y + 25, 12, MODERN_ERROR);

    int contentY = dialogRect.y + 90;
    Rectangle instrRect = {dialogRect.x + 20, contentY, dialogRect.width - 40, 50};
    DrawRectangleRounded(instrRect, 0.08f, 12, (Color){99, 102, 241, 20});
    DrawRectangleRoundedLines(instrRect, 0.08f, 12, MODERN_ACCENT);
    
    DrawText("Supported formats:", instrRect.x + 15, instrRect.y + 10, 12, MODERN_ACCENT);
    DrawText("Full URLs, youtu.be links, video IDs, playlist URLs", instrRect.x + 15, instrRect.y + 28, 11, MODERN_TEXT_LIGHT);
    
    contentY += 70;
    DrawText("Video URL", dialogRect.x + 30, contentY, 14, MODERN_TEXT);
    Rectangle urlRect = {dialogRect.x + 30, contentY + 20, 380, 40};
    DrawRectangleRounded(urlRect, 0.06f, 8, MODERN_SURFACE_2);
    DrawRectangleRoundedLines(urlRect, 0.06f, 2, ytUrlEditMode ? MODERN_ACCENT : MODERN_BORDER);
    
    if (GuiTextBox(urlRect, ytUrlBuffer, 511, ytUrlEditMode)) {
        ytUrlEditMode = !ytUrlEditMode;
    }
    
    Rectangle downloadRect = {dialogRect.x + 420, contentY + 20, 100, 40};
    const char* downloadButtonText = isDownloading ? "Downloading..." : "🎵 Download";
    Color downloadButtonColor = isDownloading ? MODERN_TEXT_LIGHT : MODERN_SUCCESS;
    
    if (isDownloading) {
        float spinTime = GetTime() * 4.0f;
        Vector2 center = {downloadRect.x + downloadRect.width/2, downloadRect.y + downloadRect.height/2};
        DrawRing(center, 8, 12, 0, spinTime * 90.0f, 16, MODERN_ACCENT);
    }
    
    if (DrawEnhancedButton(downloadRect, downloadButtonText, downloadButtonColor, MODERN_DARK, 5)) {
        if (!isDownloading && strlen(ytUrlBuffer) > 0) {
            DownloadFromYoutube(ytUrlBuffer);
        }
    }
    
    contentY += 80;
    Rectangle infoRect = {dialogRect.x + 20, contentY, dialogRect.width - 40, 80};
    DrawRectangleRounded(infoRect, 0.08f, 12, MODERN_SURFACE_2);
    DrawRectangleRoundedLines(infoRect, 0.08f, 1, MODERN_BORDER);
    
    DrawText("Information:", infoRect.x + 15, infoRect.y + 15, 12, MODERN_ACCENT);
    DrawText("Downloaded audio will be automatically selected", infoRect.x + 20, infoRect.y + 35, 11, MODERN_TEXT_LIGHT);
    DrawText("Requires yt-dlp and yt_downloader.js in directory", infoRect.x + 20, infoRect.y + 50, 11, MODERN_TEXT_LIGHT);
    
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        Rectangle statusRect = {dialogRect.x + 20, dialogRect.y + dialogRect.height - 70, dialogRect.width - 40, 45};
        Color statusColor = strstr(statusMessage, "Error") ? MODERN_ERROR : MODERN_SUCCESS;
        
        DrawRectangleRounded(statusRect, 0.08f, 12, (Color){statusColor.r, statusColor.g, statusColor.b, 20});
        DrawRectangleRoundedLines(statusRect, 0.0812, 1, statusColor);
        
        DrawText("Status:", statusRect.x + 15, statusRect.y + 8, 12, statusColor);
        
        const char* status = statusMessage;
        int maxWidth = statusRect.width - 30;
        if (MeasureText(status, 11) > maxWidth) {
            char line1[128], line2[128];
            int splitPoint = strlen(status) / 2;
            while (splitPoint > 0 && status[splitPoint] != ' ') splitPoint--;
            if (splitPoint == 0) splitPoint = strlen(status) / 2;
            
            strncpy(line1, status, splitPoint);
            line1[splitPoint] = '\0';
            strcpy(line2, status + splitPoint + 1);
            
            DrawText(line1, statusRect.x + 15, statusRect.y + 25, 11, MODERN_TEXT);
            DrawText(line2, statusRect.x + 15, statusRect.y + 37, 11, MODERN_TEXT);
        } else {
            DrawText(status, statusRect.x + 15, statusRect.y + 25, 11, MODERN_TEXT);
        }
    }
    
    Rectangle closeRect = {dialogRect.x + dialogRect.width - 80, dialogRect.y + dialogRect.height - 25, 60, 30};
    if (DrawEnhancedButton(closeRect, "Close", MODERN_TEXT_LIGHT, MODERN_ERROR, 6)) {
        showYTDialog = false;
        ytUrlBuffer[0] = '\0';
        dialogAnimTime = 0.0f;
    }
}

void DrawChat() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    pulseTime += GetFrameTime();
    for (int i = 0; i < screenHeight; i++) {
        float t = (float)i / screenHeight;
        Color color = {
            (unsigned char)(248 - t * 20),
            (unsigned char)(250 - t * 25), 
            (unsigned char)(252 - t * 30),
            255
        };
        DrawRectangle(0, i, screenWidth, 1, color);
    }
    
    Rectangle headerRect = {0, 0, screenWidth, 80};
    DrawRectangleGradientV(0, 0, screenWidth, 80, MODERN_ACCENT, (Color){MODERN_ACCENT.r, MODERN_ACCENT.g, MODERN_ACCENT.b, 200});
    DrawRectangle(0, 70, screenWidth, 10, (Color){255, 255, 255, 30});
    float pulseScale = 1.0f + sinf(pulseTime * 2.0f) * 0.05f;
    Color avatarColor = connection.isConnected ? MODERN_SUCCESS : MODERN_ERROR;
    
    if (connection.isConnected) {
        DrawRing((Vector2){50, 40}, 25, 28, 0, pulseTime * 45.0f, 32, (Color){avatarColor.r, avatarColor.g, avatarColor.b, 50});
    }
    
    DrawCircle(50, 40, 22 * pulseScale, (Color){255, 255, 255, 40});
    DrawCircle(50, 40, 20, avatarColor);
    DrawCircle(50, 40, 18, WHITE);
    DrawText(" ", 42, 32, 16, MODERN_ACCENT);
    const char* contactName = connection.isConnected ? "Secure Contact" : "Disconnected";
    const char* contactStatus = connection.isConnected ? "Online & Encrypted" : "Offline";
    
    DrawText(contactName, 85, 25, 18, WHITE);
    DrawText(contactStatus, 85, 48, 12, (Color){255, 255, 255, 200});
    if (connection.isConnected) {
        char connInfo[100];
        sprintf(connInfo, "%s:%d", connection.remoteIP, connection.remotePort);
        int textWidth = MeasureText(connInfo, 12);
        Rectangle connBadge = {screenWidth - textWidth - 25, 20, textWidth + 10, 25};
        DrawRectangleRounded(connBadge, 0.5f, 12, (Color){255, 255, 255, 30});
        DrawText(connInfo, connBadge.x + 5, connBadge.y + 7, 11, WHITE);
    }
    if (connection.isConnected) {
        Rectangle disconnectRect = {screenWidth - 120, 50, 100, 25};
        if (DrawEnhancedButton(disconnectRect, "Disconnect", MODERN_ERROR, MODERN_DARK, 7)) {
            CloseConnection();
            showConnectionDialog = true;
        }
    }
    
    int chatAreaY = 80;
    int chatAreaHeight = screenHeight - 200;
    int messageY = chatAreaY + 15 - (int)scrollOffset;
    
    if (messageCount > 0) {
        float scrollbarHeight = chatAreaHeight * 0.8f;
        float scrollbarY = chatAreaY + 10;
        float scrollRatio = scrollOffset / (messageCount * 150.0f);
        float thumbY = scrollbarY + scrollRatio * (scrollbarHeight - 40);
        
        DrawRectangleRounded((Rectangle){screenWidth - 8, scrollbarY, 6, scrollbarHeight}, 0.5f, 8, (Color){200, 200, 200, 100});
        DrawRectangleRounded((Rectangle){screenWidth - 8, thumbY, 6, 40}, 0.5f, 8, MODERN_ACCENT);
    }
    
    pthread_mutex_lock(&messageMutex);
    for (int i = 0; i < messageCount; i++) {
        ChatMessage* msg = &messages[i];
        
        int msgWidth = 380;
        int msgX = msg->isSent ? screenWidth - msgWidth - 30 : 30;
        
        int msgHeight = 70;
        if (msg->type == MSG_IMAGE || msg->type == MSG_AUDIO) {
            msgHeight = 140;
        }
        
        if (messageY + msgHeight < chatAreaY || messageY > chatAreaY + chatAreaHeight) {
            messageY += msgHeight + 20;
            continue;
        }
        
        Rectangle shadowRect = {msgX + 3, messageY + 3, msgWidth, msgHeight};
        DrawRectangleRounded(shadowRect, 0.15f, 16, (Color){0, 0, 0, 15});
        
        Color bubbleColor = msg->isSent ? 
            (Color){99, 102, 241, 240} :
            (Color){255, 255, 255, 250}; 
        
        Rectangle bubbleRect = {msgX, messageY, msgWidth, msgHeight};
        DrawRectangleRounded(bubbleRect, 0.15f, 16, bubbleColor);
        
        Color borderColor = msg->isSent ? (Color){255, 255, 255, 80} : MODERN_BORDER;
        DrawRectangleRoundedLines(bubbleRect, 0.1516, 1, borderColor);
        
        Vector2 tailPoints[3];
        if (msg->isSent) {
            tailPoints[0] = (Vector2){msgX + msgWidth - 20, messageY + msgHeight - 10};
            tailPoints[1] = (Vector2){msgX + msgWidth + 8, messageY + msgHeight - 5};
            tailPoints[2] = (Vector2){msgX + msgWidth - 15, messageY + msgHeight};
        } else {
            tailPoints[0] = (Vector2){msgX + 20, messageY + msgHeight - 10};
            tailPoints[1] = (Vector2){msgX - 8, messageY + msgHeight - 5};
            tailPoints[2] = (Vector2){msgX + 15, messageY + msgHeight};
        }
        DrawTriangle(tailPoints[0], tailPoints[1], tailPoints[2], bubbleColor);
        Color textColor = msg->isSent ? WHITE : MODERN_TEXT;
        
        if (msg->type == MSG_TEXT) {
            DrawText(msg->content, msgX + 20, messageY + 20, 14, textColor);
        } else if (msg->type == MSG_IMAGE) {
            DrawText(" ", msgX + 20, messageY + 15, 20, textColor);
            DrawText("Image", msgX + 50, messageY + 20, 14, textColor);
            
            char imagePath[512];
            if (msg->isSent) {
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                strcpy(imagePath, filename);
            } else {
                const char* filename = strrchr(msg->content, ']');
                filename = filename ? filename + 2 : msg->content;
                sprintf(imagePath, "received_%s", filename);
            }
            
            if (FileExists(imagePath)) {
                if (!messageImageLoaded[i]) {
                    Image img = LoadImage(imagePath);
                    if (img.data != NULL) {
                        float scale = fminf(180.0f / img.width, 90.0f / img.height);
                        int newWidth = (int)(img.width * scale);
                        int newHeight = (int)(img.height * scale);
                        ImageResize(&img, newWidth, newHeight);
                        
                        messageImages[i] = LoadTextureFromImage(img);
                        UnloadImage(img);
                        messageImageLoaded[i] = true;
                    }
                }
                
                if (messageImageLoaded[i]) {
                    Rectangle imgRect = {msgX + 20, messageY + 45, messageImages[i].width, messageImages[i].height};
                    DrawRectangle(imgRect.x + 2, imgRect.y + 2, imgRect.width, imgRect.height, (Color){0, 0, 0, 30});
                    DrawTexture(messageImages[i], imgRect.x, imgRect.y, WHITE);
                    DrawRectangleLinesEx(imgRect, 2, (Color){255, 255, 255, 100});
                    if (CheckCollisionPointRec(GetMousePosition(), imgRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedMessageIndex = i;
                        LoadImageForDisplay(imagePath);
                        showDecodeDialog = true;
                    }
                }
            }
            
            Rectangle decodeRect = {msgX + msgWidth - 90, messageY + msgHeight - 40, 80, 30};
            DrawEnhancedButton(decodeRect, "🔓 Decode", MODERN_WARNING, MODERN_ERROR, 8 + i);
            if (CheckCollisionPointRec(GetMousePosition(), decodeRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                selectedMessageIndex = i;
                showDecodeDialog = true;
            }
        } else if (msg->type == MSG_AUDIO) {
            DrawText(" ", msgX + 20, messageY + 15, 20, textColor);
            DrawText("Audio Message", msgX + 50, messageY + 20, 14, textColor);
            
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
            
            if (!messageAudioLoaded[i] && FileExists(audioPath)) {
                messageAudioSounds[i] = LoadSound(audioPath);
                if (IsSoundValid(messageAudioSounds[i])) {
                    messageAudioLoaded[i] = true;
                }
            }
            
            if (messageAudioLoaded[i]) {
                Rectangle playRect = {msgX + 20, messageY + 50, 60, 35};
                const char* playText = audioPlaying[i] ? "Stop" : "Play";
                Color playColor = audioPlaying[i] ? MODERN_ERROR : MODERN_SUCCESS;
                
                if (DrawEnhancedButton(playRect, playText, playColor, MODERN_DARK, 9 + i)) {
                    if (audioPlaying[i]) {
                        StopSound(messageAudioSounds[i]);
                        audioPlaying[i] = false;
                    } else {
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
                if (audioPlaying[i]) {
                    for (int bar = 0; bar < 8; bar++) {
                        float barHeight = 5 + sinf(pulseTime * 8.0f + bar) * 15;
                        DrawRectangle(msgX + 90 + bar * 8, messageY + 70 - barHeight, 4, barHeight, 
                                    (Color){MODERN_SUCCESS.r, MODERN_SUCCESS.g, MODERN_SUCCESS.b, 150});
                    }
                }
                
                if (audioPlaying[i] && !IsSoundPlaying(messageAudioSounds[i])) {
                    audioPlaying[i] = false;
                }
            } else {
                Rectangle audioPlaceholder = {msgX + 20, messageY + 50, 150, 35};
                DrawRectangleRounded(audioPlaceholder, 0.1f, 8, (Color){MODERN_ACCENT.r, MODERN_ACCENT.g, MODERN_ACCENT.b, 30});
                DrawText("Audio File", audioPlaceholder.x + 10, audioPlaceholder.y + 12, 12, textColor);
            }
            
            Rectangle decodeRect = {msgX + msgWidth - 90, messageY + msgHeight - 40, 80, 30};
            DrawEnhancedButton(decodeRect, "Decode", MODERN_WARNING, MODERN_ERROR, 10 + i);
            if (CheckCollisionPointRec(GetMousePosition(), decodeRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                selectedMessageIndex = i;
                showDecodeDialog = true;
            }
        }
        if (msg->hasHiddenMessage) {
            float glowPulse = (sinf(pulseTime * 4.0f) + 1.0f) / 2.0f;
            DrawCircle(msgX + 15, messageY + 15, 8, (Color){255, 193, 7, (unsigned char)(100 + glowPulse * 50)});
            DrawCircle(msgX + 15, messageY + 15, 6, MODERN_WARNING);
            DrawText(" ", msgX + 11, messageY + 11, 8, WHITE);
        }
        
        Rectangle timeRect = {msgX + msgWidth - 70, messageY + msgHeight - 25, 65, 20};
        DrawRectangleRounded(timeRect, 0.5f, 8, (Color){0, 0, 0, 30});
        DrawText(msg->timestamp, timeRect.x + 5, timeRect.y + 5, 9, (Color){255, 255, 255, 180});
        
        messageY += msgHeight + 20;
    }
    pthread_mutex_unlock(&messageMutex);
    float wheel = GetMouseWheelMove();
    scrollOffset -= wheel * 50;
    if (scrollOffset < 0) scrollOffset = 0;
    int inputAreaY = screenHeight - 180;
    Rectangle inputBg = {0, inputAreaY, screenWidth, 180};
    DrawRectangle(0, inputAreaY, screenWidth, 180, (Color){255, 255, 255, 240});
    DrawRectangle(0, inputAreaY, screenWidth, 2, MODERN_ACCENT);
    int buttonY = inputAreaY + 15;
    int buttonWidth = 85;
    int buttonSpacing = 95;
    int startX = 20;
    if (DrawEnhancedButton((Rectangle){startX, buttonY, buttonWidth, 30}, "Gen Image", MODERN_ACCENT, MODERN_DARK, 11)) {
        GenerateRandomImage();
    }
    
    if (DrawEnhancedButton((Rectangle){startX + buttonSpacing, buttonY, buttonWidth, 30}, "Gen Audio", MODERN_SUCCESS, MODERN_DARK, 12)) {
        GenerateRandomAudio();
    }
    
    if (DrawEnhancedButton((Rectangle){startX + buttonSpacing * 2, buttonY, buttonWidth, 30}, "YT Download", MODERN_ERROR, MODERN_DARK, 13)) {
        showYTDialog = true;
    }
    
    bool canEncode = strlen(selectedFilePath) > 0 && strlen(hiddenMessageBuffer) > 0;
    Color encodeColor = canEncode ? MODERN_WARNING : MODERN_TEXT_LIGHT;
    if (DrawEnhancedButton((Rectangle){startX + buttonSpacing * 3, buttonY, buttonWidth, 30}, "Encode", encodeColor, MODERN_DARK, 14)) {
        if (canEncode) {
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
    
    bool canSendEncoded = connection.isConnected && strlen(selectedFilePath) > 0 && FileExists(selectedFilePath);
    Color sendEncodedColor = canSendEncoded ? MODERN_SUCCESS : MODERN_TEXT_LIGHT;
    if (DrawEnhancedButton((Rectangle){startX + buttonSpacing * 4, buttonY, buttonWidth, 30}, "Send Encoded", sendEncodedColor, MODERN_DARK, 15)) {
        if (canSendEncoded) {
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
    
    if (DrawEnhancedButton((Rectangle){startX + buttonSpacing * 5, buttonY, buttonWidth, 30}, "Decode File", MODERN_WARNING, MODERN_DARK, 16)) {
        showDecodeDialog = true;
        selectedMessageIndex = -1;
        dialogAnimTime = 0.0f;
    }
    
    buttonY += 45;
    DrawText("Message Type:", startX, buttonY + 8, 12, MODERN_TEXT);
    
    Rectangle textTypeRect = {startX + 100, buttonY, 70, 30};
    Rectangle imageTypeRect = {startX + 180, buttonY, 70, 30};
    Rectangle audioTypeRect = {startX + 260, buttonY, 70, 30};
    
    Color textColor = selectedMessageType == MSG_TEXT ? MODERN_ACCENT : MODERN_TEXT_LIGHT;
    Color imageColor = selectedMessageType == MSG_IMAGE ? MODERN_SUCCESS : MODERN_TEXT_LIGHT;
    Color audioColor = selectedMessageType == MSG_AUDIO ? MODERN_WARNING : MODERN_TEXT_LIGHT;
    
    if (DrawEnhancedButton(textTypeRect, "Text", textColor, MODERN_DARK, 17)) {
        selectedMessageType = MSG_TEXT;
    }
    if (DrawEnhancedButton(imageTypeRect, "Image", imageColor, MODERN_DARK, 18)) {
        selectedMessageType = MSG_IMAGE;
    }
    if (DrawEnhancedButton(audioTypeRect, "Audio", audioColor, MODERN_DARK, 19)) {
        selectedMessageType = MSG_AUDIO;
    }
    
    if (selectedMessageType != MSG_TEXT) {
        buttonY += 40;
        DrawText("File Path:", startX, buttonY + 8, 12, MODERN_TEXT);
        Rectangle fileRect = {startX + 80, buttonY, 450, 30};
        DrawRectangleRounded(fileRect, 0.06f, 8, MODERN_SURFACE_2);
        DrawRectangleRoundedLines(fileRect, 0.06f, 1, filePathEditMode ? MODERN_ACCENT : MODERN_BORDER);
        
        if (GuiTextBox(fileRect, selectedFilePath, 255, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
        
        bool fileExists = strlen(selectedFilePath) > 0 && FileExists(selectedFilePath);
        DrawCircle(startX + 540, buttonY + 15, 6, fileExists ? MODERN_SUCCESS : MODERN_ERROR);
        
        if (DrawEnhancedButton((Rectangle){startX + 550, buttonY, 70, 30}, "Browse", MODERN_ACCENT, MODERN_DARK, 20)) {
            ShowStatus("Use file picker or type path manually");
        }
    }
    
    if (selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) {
        buttonY += 40;
        DrawText("Hidden Message:", startX, buttonY + 8, 12, MODERN_TEXT_LIGHT);
        Rectangle hiddenRect = {startX + 120, buttonY, screenWidth - 160, 30};
        DrawRectangleRounded(hiddenRect, 0.06f, 8, (Color){255, 248, 225, 255});
        DrawRectangleRoundedLines(hiddenRect, 0.068, 1, hiddenMessageEditMode ? MODERN_WARNING : (Color){255, 193, 7, 255});
        
        if (GuiTextBox(hiddenRect, hiddenMessageBuffer, MAX_MESSAGE_LENGTH - 1, hiddenMessageEditMode)) {
            hiddenMessageEditMode = !hiddenMessageEditMode;
        }
    }
    
    buttonY += 40;
    DrawText("Message:", startX, buttonY + 8, 14, MODERN_TEXT);
    Rectangle msgRect = {startX + 90, buttonY, screenWidth - 200, 35};
    DrawRectangleRounded(msgRect, 0.08f, 12, MODERN_SURFACE);
    DrawRectangleRoundedLines(msgRect, 0.08f,  2, inputEditMode ? MODERN_ACCENT : MODERN_BORDER);
    
    if (GuiTextBox(msgRect, inputBuffer, MAX_MESSAGE_LENGTH - 1, inputEditMode)) {
        inputEditMode = !inputEditMode;
    }
    
    bool canSend = false;
    if (selectedMessageType == MSG_TEXT && strlen(inputBuffer) > 0) {
        canSend = true;
    } else if ((selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) && 
               strlen(selectedFilePath) > 0 && FileExists(selectedFilePath)) {
        canSend = true;
    }
    
    Rectangle sendRect = {screenWidth - 90, buttonY, 70, 35};
    Color sendColor = canSend && connection.isConnected ? MODERN_SUCCESS : MODERN_TEXT_LIGHT;
    if (canSend && connection.isConnected) {
        float sendPulse = (sinf(pulseTime * 3.0f) + 1.0f) / 2.0f;
        DrawRing((Vector2){sendRect.x + sendRect.width/2, sendRect.y + sendRect.height/2}, 
                25, 30, 0, 360, 32, (Color){sendColor.r, sendColor.g, sendColor.b, (unsigned char)(30 * sendPulse)});
    }
    
    if (DrawEnhancedButton(sendRect, "Send", sendColor, MODERN_DARK, 21)) {
        if (canSend && connection.isConnected) {
            if (selectedMessageType == MSG_TEXT) {
                SendMessage(inputBuffer, MSG_TEXT);
                inputBuffer[0] = '\0';
            } else {
                SendFile(selectedFilePath, selectedMessageType);
                selectedFilePath[0] = '\0';
                hiddenMessageBuffer[0] = '\0';
            }
        } else if (!connection.isConnected) {
            ShowStatus("Not connected!");
        }
    }
    
    if (statusTimer > 0) {
        Rectangle statusRect = {20, inputAreaY - 40, screenWidth - 40, 30};
        Color statusBgColor = strstr(statusMessage, "Error") || strstr(statusMessage, "") ? MODERN_ERROR :
                             strstr(statusMessage, "Warning") || strstr(statusMessage, "") ? MODERN_WARNING : MODERN_SUCCESS;
        
        float statusAlpha = fminf(statusTimer / 2.0f, 1.0f);
        statusBgColor.a = (unsigned char)(statusBgColor.a * statusAlpha);
        
        DrawRectangleRounded(statusRect, 0.08f, 12, (Color){statusBgColor.r, statusBgColor.g, statusBgColor.b, 30});
        DrawRectangleRoundedLines(statusRect, 0.0812, 1, statusBgColor);
        
        int textX = statusRect.x + (statusRect.width - MeasureText(statusMessage, 12)) / 2;
        DrawText(statusMessage, textX, statusRect.y + 9, 12, statusBgColor);
    }
    
    if (showDecodeDialog) {
        DrawDecodeDialog();
    }
    
    if (showYTDialog) {
        DrawYTDialog();
    }
}