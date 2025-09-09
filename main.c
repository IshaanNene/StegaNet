#include "common.h"
#include <string.h>
#include "network.h"
#include "ui.h"
#include "utils.h"

ChatMessage messages[MAX_MESSAGES];
int messageCount = 0;
ConnectionInfo connection = {0};
char inputBuffer[MAX_MESSAGE_LENGTH] = "";
char hiddenMessageBuffer[MAX_MESSAGE_LENGTH] = "";
bool inputEditMode = false;
bool hiddenMessageEditMode = false;
float scrollOffset = 0;
bool isServer = false;
FileTransfer currentTransfer = {0};
pthread_mutex_t messageMutex = PTHREAD_MUTEX_INITIALIZER;
bool showConnectionDialog = true;
char serverIPBuffer[20] = "127.0.0.1";
char portBuffer[10] = "8888";
bool serverIPEditMode = false;
bool portEditMode = false;
char statusMessage[256] = "";
float statusTimer = 0;
bool showEncodingOptions = false;
MessageType selectedMessageType = MSG_TEXT;
char selectedFilePath[256] = "";
bool filePathEditMode = false;
bool shouldExit = false;
Texture2D currentImageTexture = {0};
bool imageLoaded = false;
bool showDecodeDialog = false;
int selectedMessageIndex = -1;
char decodeFilePath[256] = "";
bool decodeFilePathEditMode = false;
char decodedHiddenMessage[MAX_MESSAGE_LENGTH] = "";
Texture2D messageImages[MAX_MESSAGES] = {0};
bool messageImageLoaded[MAX_MESSAGES] = {0};
Sound messageAudioSounds[MAX_MESSAGES] = {0};
bool messageAudioLoaded[MAX_MESSAGES] = {0};
bool audioPlaying[MAX_MESSAGES] = {0};
bool showYTDialog = false;
char ytUrlBuffer[512] = "https://www.youtube.com/";
bool ytUrlEditMode = false;
bool isDownloading = false;

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(800, 600, "StegaChat - Secure Messaging with Steganography");

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    SetTargetFPS(60);

    while (!WindowShouldClose() && !shouldExit) {
        if (statusTimer > 0) statusTimer -= GetFrameTime();

        if (IsKeyPressed(KEY_ENTER) && !showConnectionDialog && connection.isConnected && !inputEditMode) {
            if (selectedMessageType == MSG_TEXT && strlen(inputBuffer) > 0) {
                SendMessage(inputBuffer, MSG_TEXT);
                inputBuffer[0] = '\0';
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (showEncodingOptions) {
                showEncodingOptions = false;
            } else if (!showConnectionDialog && connection.isConnected) {
                CloseConnection();
                showConnectionDialog = true;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (showConnectionDialog) {
            DrawConnectionDialog();
        } else if (connection.isConnected || messageCount > 0) {
            DrawChat();
        } else {
            showConnectionDialog = true;
        }

        EndDrawing();
    }

    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (messageImageLoaded[i]) UnloadTexture(messageImages[i]);
        if (messageAudioLoaded[i]) UnloadSound(messageAudioSounds[i]);
    }

    CloseConnection();
    pthread_mutex_destroy(&messageMutex);
    if (imageLoaded) UnloadTexture(currentImageTexture);
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();

    return 0;
}
