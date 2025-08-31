#include "globals.h"
#include <string.h>
#include <raylib.h>

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
        DrawText(statusMessage, screenWidth / 2 - textWidth / 2, 30, 16, BLUE);
    }
}
