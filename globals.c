#include "globals.h"

AppState currentState = STATE_MAIN_MENU;
ImageSource selectedImageSource = IMAGE_NONE;
AudioSource selectedAudioSource = AUDIO_NONE;

char ytLinkBuffer[256] = "";
char messageBuffer[512] = "";
char decodedMessageBuffer[512] = "";
char filePathBuffer[256] = "";

bool ytLinkEditMode = false;
bool messageEditMode = false;
bool filePathEditMode = false;

Texture2D encodedTexture;
bool encodedTextureLoaded = false;
Wave audioWave;
bool audioWaveLoaded = false;
Sound audioSound;
bool audioSoundLoaded = false;
bool isPlayingAudio = false;

char statusMessage[256] = "";
float statusMessageTimer = 0;
