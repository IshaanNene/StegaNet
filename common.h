#ifndef COMMON_H
#define COMMON_H

#include <raylib.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 512
#define MAX_CLIENTS 10
#define PORT 8888
#define BUFFER_SIZE 1048576

typedef enum {
    MSG_TEXT,
    MSG_IMAGE,
    MSG_AUDIO,
    MSG_FILE_CHUNK,
    MSG_FILE_END
} MessageType;

typedef struct {
    char sender[50];
    char content[MAX_MESSAGE_LENGTH];
    char timestamp[20];
    MessageType type;
    bool isSent;
    char filename[256];
    bool hasHiddenMessage;
    char hiddenMessage[MAX_MESSAGE_LENGTH];
} ChatMessage;

typedef struct {
    char localIP[20];
    char remoteIP[20];
    int localPort;
    int remotePort;
    bool isConnected;
    int socket_fd;
    pthread_t receiveThread;
    bool threadActive;
} ConnectionInfo;

typedef struct {
    char filename[256];
    unsigned char* data;
    size_t size;
    size_t received;
    bool isReceiving;
    MessageType fileType;
} FileTransfer;

extern ChatMessage messages[MAX_MESSAGES];
extern int messageCount;
extern ConnectionInfo connection;
extern char inputBuffer[MAX_MESSAGE_LENGTH];
extern char hiddenMessageBuffer[MAX_MESSAGE_LENGTH];
extern bool inputEditMode;
extern bool hiddenMessageEditMode;
extern float scrollOffset;
extern bool isServer;
extern FileTransfer currentTransfer;
extern pthread_mutex_t messageMutex;
extern bool showConnectionDialog;
extern char serverIPBuffer[20];
extern char portBuffer[10];
extern bool serverIPEditMode;
extern bool portEditMode;
extern char statusMessage[256];
extern float statusTimer;
extern bool showEncodingOptions;
extern MessageType selectedMessageType;
extern char selectedFilePath[256];
extern bool filePathEditMode;
extern bool shouldExit;
extern Texture2D currentImageTexture;
extern bool imageLoaded;
extern bool showDecodeDialog;
extern int selectedMessageIndex;
extern char decodeFilePath[256];
extern bool decodeFilePathEditMode;
extern char decodedHiddenMessage[MAX_MESSAGE_LENGTH];
extern Texture2D messageImages[MAX_MESSAGES];
extern bool messageImageLoaded[MAX_MESSAGES];
extern Sound messageAudioSounds[MAX_MESSAGES];
extern bool messageAudioLoaded[MAX_MESSAGES];
extern bool audioPlaying[MAX_MESSAGES];
extern bool showYTDialog;
extern char ytUrlBuffer[512];
extern bool ytUrlEditMode;
extern bool isDownloading;

#endif
