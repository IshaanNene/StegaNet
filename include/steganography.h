#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

#include "common.h"

void EncodeMessageInImage(AppState *state, const char *imagePath,
                          const char *message, const char *outputPath);
char *DecodeMessageFromImage(AppState *state, const char *imagePath);
void EncodeMessageInAudio(AppState *state, const char *audioPath,
                          const char *message, const char *outputPath);
char *DecodeMessageFromAudio(AppState *state, const char *audioPath);

#endif
