#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>

typedef enum {
  ERR_NONE = 0,
  ERR_INVALID_INPUT,
  ERR_NETWORK,
  ERR_MEMORY,
  ERR_FILE_NOT_FOUND,
  ERR_CAPACITY_EXCEEDED,
  ERR_DECODE_FAILED
} ErrorCode;

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 4096
#define MAX_CLIENTS 10
#define PORT 8888
#define BUFFER_SIZE 1048576

typedef enum {
  MSG_TEXT,
  MSG_IMAGE,
  MSG_AUDIO,
  MSG_FILE_CHUNK,
  MSG_FILE_END,
  MSG_PING,
  MSG_PONG
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
  float lastPingSent;
  float lastPongReceived;
  float latency;
} ConnectionInfo;

typedef struct {
  char filename[256];
  unsigned char *data;
  size_t size;
  size_t received;
  bool isReceiving;
  MessageType fileType;
} FileTransfer;

typedef struct {
  ChatMessage messages[MAX_MESSAGES];
  int messageCount;
  ConnectionInfo connection;
  char inputBuffer[MAX_MESSAGE_LENGTH];
  char hiddenMessageBuffer[MAX_MESSAGE_LENGTH];
  bool inputEditMode;
  bool hiddenMessageEditMode;
  float scrollOffset;
  bool isServer;
  FileTransfer currentTransfer;
  pthread_mutex_t messageMutex;
  bool showConnectionDialog;
  char serverIPBuffer[20];
  char portBuffer[10];
  bool serverIPEditMode;
  bool portEditMode;
  char statusMessage[256];
  float statusTimer;
  bool showEncodingOptions;
  MessageType selectedMessageType;
  char selectedFilePath[256];
  bool filePathEditMode;
  bool shouldExit;
  Texture2D currentImageTexture;
  bool imageLoaded;
  bool showDecodeDialog;
  int selectedMessageIndex;
  char decodeFilePath[256];
  bool decodeFilePathEditMode;
  char decodedHiddenMessage[MAX_MESSAGE_LENGTH];
  Texture2D messageImages[MAX_MESSAGES];
  bool messageImageLoaded[MAX_MESSAGES];
  Sound messageAudioSounds[MAX_MESSAGES];
  bool messageAudioLoaded[MAX_MESSAGES];
  bool audioPlaying[MAX_MESSAGES];
  bool showYTDialog;
  char ytUrlBuffer[512];
  bool ytUrlEditMode;
  bool isDownloading;

  // Encryption state
  bool useEncryption;
  char encryptionKey[64];
  bool encryptionKeyEditMode;

  // Search Filter
  char filterBuffer[256];
  bool filterEditMode;

  Sound alertSound;
} AppState;

#endif
