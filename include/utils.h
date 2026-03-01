#ifndef UTILS_H
#define UTILS_H

#include "common.h"

void AddMessage(AppState *state, const char *sender, const char *content,
                MessageType type, bool isSent);
void ShowStatus(AppState *state, const char *message);
void GenerateRandomImage(AppState *state);
void GenerateRandomAudio(AppState *state);
void DownloadFromYoutube(AppState *state, const char *url);
void LoadImageForDisplay(AppState *state, const char *imagePath);

#endif
