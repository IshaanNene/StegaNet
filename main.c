#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "globals.h"
#include "utils.h"
#include "draw.h"

int main() {
    InitWindow(1000, 600, "StegaNet");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateStatusMessage();

        BeginDrawing();
        switch (currentState) {
            case STATE_MAIN_MENU: DrawMainMenu(); break;
            case STATE_IMAGE_STEGO: DrawImageSteganography(); break;
            case STATE_IMAGE_OPTIONS: DrawImageOptions(); break;
            case STATE_IMAGE_ENCODE: DrawImageEncode(); break;
            case STATE_IMAGE_DECODE: DrawImageDecode(); break;
            case STATE_AUDIO_STEGO: DrawAudioOptions(); break;
            case STATE_AUDIO_OPTIONS: DrawAudioSteganography(); break;
            case STATE_AUDIO_ENCODE: DrawAudioEncode(); break;
            case STATE_AUDIO_DECODE: DrawAudioDecode(); break;
            default: break;
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
