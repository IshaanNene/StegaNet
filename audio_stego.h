#ifndef AUDIO_STEGO_H
#define AUDIO_STEGO_H

void GenerateRandomAudio();
void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath);
char* DecodeMessageFromAudio(const char* audioPath);

#endif
