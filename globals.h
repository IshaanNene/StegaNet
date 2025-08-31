#ifndef GLOBALS_H
#define GLOBALS_H

#include <raylib.h>
#include "state.h"

// Global state
extern AppState currentState;
extern ImageSource selectedImageSource;
extern AudioSource selectedAudioSource;

extern char ytLinkBuffer[256];
extern char messageBuffer[512];
extern char decodedMessageBuffer[512];
extern char filePathBuffer[256];

extern bool ytLinkEditMode;
extern bool messageEditMode;
extern bool filePathEditMode;

extern Texture2D encodedTexture;
extern bool encodedTextureLoaded;
extern Wave audioWave;
extern bool audioWaveLoaded;
extern Sound audioSound;
extern bool audioSoundLoaded;
extern bool isPlayingAudio;

extern char statusMessage[256];
extern float statusMessageTimer;

#endif
