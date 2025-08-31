#ifndef IMAGE_STEGO_H
#define IMAGE_STEGO_H

void GenerateRandomImage();
void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath);
char* DecodeMessageFromImage(const char* imagePath);

#endif
