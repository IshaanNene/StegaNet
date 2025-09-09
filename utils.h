#ifndef UTILS_H
#define UTILS_H

#include "common.h"

void AddMessage(const char* sender, const char* content, MessageType type, bool isSent);
void ShowStatus(const char* message);
void GenerateRandomImage();
void GenerateRandomAudio();
void DownloadFromYoutube(const char* url);
void LoadImageForDisplay(const char* imagePath);

#endif
