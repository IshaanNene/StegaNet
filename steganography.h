#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

#include "common.h"

void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath);
char* DecodeMessageFromImage(const char* imagePath);
void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath);
char* DecodeMessageFromAudio(const char* audioPath);

#endif
