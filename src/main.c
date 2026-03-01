#include "common.h"
#include "logging.h"
#include "network.h"
#include "ui.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// no global variables needed, we will initialize in main.

int main(void) {
  Logger_Init("steganet.log");
  SetTraceLogLevel(LOG_WARNING);

  AppState appState = {0};
  AppState *state = &appState;
  // Set some defaults
  strcpy(state->serverIPBuffer, "127.0.0.1");
  strcpy(state->portBuffer, "8888");
  state->showConnectionDialog = true;
  strcpy(state->ytUrlBuffer, "https://www.youtube.com/");
  state->messageMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(800, 600, "StegaChat - Secure Messaging with Steganography");

  if (!IsAudioDeviceReady()) {
    InitAudioDevice();
  }

  // Generate short notification beep
  Wave beepWave = {0};
  beepWave.frameCount = 44100 / 10;
  beepWave.sampleRate = 44100;
  beepWave.sampleSize = 16;
  beepWave.channels = 1;
  short *beepData = (short *)malloc(beepWave.frameCount * sizeof(short));
  for (unsigned int i = 0; i < beepWave.frameCount; i++) {
    beepData[i] =
        (short)(8000.0f * ((i % 100) < 50 ? 1 : -1)); // simple square wave
  }
  beepWave.data = beepData;
  state->alertSound = LoadSoundFromWave(beepWave);
  UnloadWave(beepWave);

  SetTargetFPS(60);

  while (!WindowShouldClose() && !state->shouldExit) {
    if (state->statusTimer > 0)
      state->statusTimer -= GetFrameTime();

    if (state->connection.isConnected) {
      if (GetTime() - state->connection.lastPingSent > 2.0f) {
        SendPing(state);
      }
    }

    bool shouldSend = false;
    if (IsKeyPressed(KEY_ENTER)) {
      if (!state->inputEditMode)
        shouldSend = true;
      else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        shouldSend = true;
    }

    if (shouldSend && !state->showConnectionDialog &&
        state->connection.isConnected) {
      if (state->selectedMessageType == MSG_TEXT &&
          strlen(state->inputBuffer) > 0) {
        SendMessage(state, state->inputBuffer, MSG_TEXT);
        state->inputBuffer[0] = '\0';
        state->inputEditMode = false;
      }
    }

    if (IsFileDropped()) {
      FilePathList droppedFiles = LoadDroppedFiles();
      if (droppedFiles.count > 0) {
        strncpy(state->selectedFilePath, droppedFiles.paths[0],
                sizeof(state->selectedFilePath) - 1);
        state->selectedFilePath[sizeof(state->selectedFilePath) - 1] = '\0';
        char *ext = strrchr(droppedFiles.paths[0], '.');
        if (ext) {
          if (strcasecmp(ext, ".png") == 0 || strcasecmp(ext, ".jpg") == 0 ||
              strcasecmp(ext, ".jpeg") == 0)
            state->selectedMessageType = MSG_IMAGE;
          else if (strcasecmp(ext, ".wav") == 0 ||
                   strcasecmp(ext, ".mp3") == 0 || strcasecmp(ext, ".ogg") == 0)
            state->selectedMessageType = MSG_AUDIO;
          else
            state->selectedMessageType = MSG_FILE_CHUNK;
        }
      }
      UnloadDroppedFiles(droppedFiles);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      if (state->showEncodingOptions) {
        state->showEncodingOptions = false;
      } else if (!state->showConnectionDialog &&
                 state->connection.isConnected) {
        CloseConnection(state);
        state->showConnectionDialog = true;
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (state->showConnectionDialog) {
      DrawConnectionDialog(state);
    } else if (state->connection.isConnected || state->messageCount > 0) {
      DrawChat(state);
    } else {
      state->showConnectionDialog = true;
    }

    EndDrawing();
  }

  for (int i = 0; i < MAX_MESSAGES; i++) {
    if (state->messageImageLoaded[i])
      UnloadTexture(state->messageImages[i]);
    if (state->messageAudioLoaded[i])
      UnloadSound(state->messageAudioSounds[i]);
  }

  CloseConnection(state);
  pthread_mutex_destroy(&state->messageMutex);
  if (state->imageLoaded)
    UnloadTexture(state->currentImageTexture);
  if (IsAudioDeviceReady())
    CloseAudioDevice();
  CloseWindow();
  Logger_Close();

  return 0;
}
