#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "logging.h"
#include "utils.h"

void AddMessage(AppState *state, const char *sender, const char *content,
                MessageType type, bool isSent) {
  if (!sender || !content)
    return;

  if (state->messageCount >= MAX_MESSAGES) {
    for (int i = 0; i < MAX_MESSAGES - 1; i++) {
      state->messages[i] = state->messages[i + 1];
    }
    state->messageCount--;
  }

  ChatMessage *msg = &state->messages[state->messageCount++];
  strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
  msg->sender[sizeof(msg->sender) - 1] = '\0';
  strncpy(msg->content, content, sizeof(msg->content) - 1);
  msg->content[sizeof(msg->content) - 1] = '\0';
  msg->type = type;
  msg->isSent = isSent;
  msg->hasHiddenMessage = false;
  msg->hiddenMessage[0] = '\0';

  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  if (tm_info) {
    strftime(msg->timestamp, sizeof(msg->timestamp), "%H:%M", tm_info);
  } else {
    strcpy(msg->timestamp, "??:??");
  }

  if (!isSent && IsAudioDeviceReady()) {
    PlaySound(state->alertSound);
  }
}

void ShowStatus(AppState *state, const char *message) {
  if (message) {
    strncpy(state->statusMessage, message, sizeof(state->statusMessage) - 1);
    state->statusMessage[sizeof(state->statusMessage) - 1] = '\0';
    state->statusTimer = 3.0f;
  }
}

void DownloadFromYoutube(AppState *state, const char *url) {
  if (state->isDownloading) {
    ShowStatus(state, "Download already in progress...");
    return;
  }

  if (strlen(url) == 0) {
    ShowStatus(state, "Please enter a YouTube URL");
    return;
  }

  // Validate URL to prevent command injection
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    ShowStatus(state, "Invalid URL format (must start with http/https)");
    return;
  }
  for (size_t i = 0; i < strlen(url); i++) {
    char c = url[i];
    if (c == '"' || c == '\'' || c == '`' || c == '$' || c == ';' || c == '|' ||
        c == '&' || c == '<' || c == '>') {
      ShowStatus(state, "Invalid characters in URL");
      return;
    }
  }

  state->isDownloading = true;
  ShowStatus(state, "Starting download...");

  char command[1024];
  char cwd[512];
  if (!getcwd(cwd, sizeof(cwd))) {
    ShowStatus(state, "Failed to get working directory");
    state->isDownloading = false;
    return;
  }

  snprintf(command, sizeof(command), "node \"%s/../yt_downloader.js\" \"%s\"",
           cwd, url);

  FILE *pipe = popen(command, "r");
  if (!pipe) {
    ShowStatus(state, "Failed to start download process");
    state->isDownloading = false;
    return;
  }

  char result[512] = {0};
  char line[256];

  while (fgets(line, sizeof(line), pipe) != NULL) {
    strncpy(result, line, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';
  }

  int exit_status = pclose(pipe);
  char *newline = strchr(result, '\n');
  if (newline)
    *newline = '\0';

  if (exit_status == 0 && strlen(result) > 0 &&
      strstr(result, "Error:") == NULL && strstr(result, "ERROR:") == NULL) {
    strncpy(state->selectedFilePath, result,
            sizeof(state->selectedFilePath) - 1);
    state->selectedFilePath[sizeof(state->selectedFilePath) - 1] = '\0';
    state->selectedMessageType = MSG_AUDIO;
    ShowStatus(state, "Download complete! Audio ready to encode/send.");
  } else if (strlen(result) > 0) {
    ShowStatus(state, result);
  } else {
    ShowStatus(state,
               "Download failed - check URL and internet state->connection");
  }

  state->isDownloading = false;
}

void GenerateRandomAudio(AppState *state) {
  LOG_INFO("Generating random audio...");

  int sampleRate = 44100;
  int duration = 5;
  int sampleCount = sampleRate * duration;

  Wave wave = {0};
  wave.frameCount = sampleCount;
  wave.sampleRate = sampleRate;
  wave.sampleSize = 16;
  wave.channels = 1;

  short *samples = (short *)malloc(sampleCount * sizeof(short));
  if (!samples) {
    ShowStatus(state, "Memory allocation failed for audio generation");
    return;
  }

  for (int i = 0; i < sampleCount; i++) {
    float t = (float)i / sampleRate;
    float frequency = 440.0f + sin(t * 2.0f) * 100.0f;
    float sample = sin(2.0f * PI * frequency * t) * 0.5f;
    sample += (GetRandomValue(-1000, 1000) / 10000.0f) * 0.1f;
    samples[i] = (short)(sample * 32767);
  }

  wave.data = samples;

  bool success = ExportWave(wave, "randomAudio.wav");
  UnloadWave(wave);

  if (success) {
    ShowStatus(state, "Random audio generated: randomAudio.wav");
    strcpy(state->selectedFilePath, "randomAudio.wav");
    state->selectedMessageType = MSG_AUDIO;
  } else {
    ShowStatus(state, "Failed to generate random audio");
  }
}

void GenerateRandomImage(AppState *state) {
  LOG_INFO("Generating random image...");

  int width = 512;
  int height = 512;
  Image image = GenImageColor(width, height, WHITE);

  if (image.data == NULL) {
    ShowStatus(state, "Failed to create image");
    return;
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Color color = {GetRandomValue(0, 255), GetRandomValue(0, 255),
                     GetRandomValue(0, 255), 255};
      ImageDrawPixel(&image, x, y, color);
    }
  }

  bool success = ExportImage(image, "randomImage.png");
  UnloadImage(image);

  if (success) {
    ShowStatus(state, "Random image generated: randomImage.png");
    strcpy(state->selectedFilePath, "randomImage.png");
    state->selectedMessageType = MSG_IMAGE;
  } else {
    ShowStatus(state, "Failed to generate random image");
  }
}
void LoadImageForDisplay(AppState *state, const char *imagePath) {
  if (state->imageLoaded) {
    UnloadTexture(state->currentImageTexture);
    state->imageLoaded = false;
  }

  if (FileExists(imagePath)) {
    Image image = LoadImage(imagePath);
    if (image.data != NULL) {
      if (image.width > 400 || image.height > 300) {
        float scale = fminf(400.0f / image.width, 300.0f / image.height);
        int newWidth = (int)(image.width * scale);
        int newHeight = (int)(image.height * scale);
        ImageResize(&image, newWidth, newHeight);
      }
      state->currentImageTexture = LoadTextureFromImage(image);
      UnloadImage(image);
      state->imageLoaded = true;
    }
  }
}
