#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 512
#define MAX_CLIENTS 10
#define PORT 8888
#define BUFFER_SIZE 1048576 // 1MB buffer for file transfers

// Message types
typedef enum {
    MSG_TEXT,
    MSG_IMAGE,
    MSG_AUDIO,
    MSG_FILE_CHUNK,
    MSG_FILE_END
} MessageType;

// Chat message structure
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

// Connection info
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

// File transfer structure
typedef struct {
    char filename[256];
    unsigned char* data;
    size_t size;
    size_t received;
    bool isReceiving;
    MessageType fileType;
} FileTransfer;

// Global variables
ChatMessage messages[MAX_MESSAGES];
int messageCount = 0;
ConnectionInfo connection = {0};
char inputBuffer[MAX_MESSAGE_LENGTH] = "";
char hiddenMessageBuffer[MAX_MESSAGE_LENGTH] = "";
bool inputEditMode = false;
bool hiddenMessageEditMode = false;
float scrollOffset = 0;
bool isServer = false;
FileTransfer currentTransfer = {0};
pthread_mutex_t messageMutex = PTHREAD_MUTEX_INITIALIZER;
bool showConnectionDialog = true;
char serverIPBuffer[20] = "127.0.0.1";
char portBuffer[10] = "8888";
bool serverIPEditMode = false;
bool portEditMode = false;
char statusMessage[256] = "";
float statusTimer = 0;
bool showEncodingOptions = false;
MessageType selectedMessageType = MSG_TEXT;
char selectedFilePath[256] = "";
bool filePathEditMode = false;
bool shouldExit = false;
Texture2D currentImageTexture = {0};
bool imageLoaded = false;
bool showDecodeDialog = false;
int selectedMessageIndex = -1;
char decodeFilePath[256] = "";
bool decodeFilePathEditMode = false;
char decodedHiddenMessage[MAX_MESSAGE_LENGTH] = "";
Texture2D messageImages[MAX_MESSAGES] = {0};
bool messageImageLoaded[MAX_MESSAGES] = {0};
Sound messageAudioSounds[MAX_MESSAGES] = {0};
bool messageAudioLoaded[MAX_MESSAGES] = {0};
bool audioPlaying[MAX_MESSAGES] = {0};
bool showYTDialog = false;
char ytUrlBuffer[512] = "https://www.youtube.com/";
bool ytUrlEditMode = false;
bool isDownloading = false;

// Function prototypes
void* ReceiveMessages(void* arg);
void SendMessage(const char* message, MessageType type);
void SendFile(const char* filepath, MessageType type);
void ProcessReceivedFile(FileTransfer* transfer);
void AddMessage(const char* sender, const char* content, MessageType type, bool isSent);
void ShowStatus(const char* message);
void InitializeConnection(bool asServer, const char* ip, int port);
void CloseConnection();
void GenerateRandomAudio();
void GenerateRandomImage();
void DownloadYouTubeAudio(const char* url);
void DownloadYouTubeAudio(const char* url) {
    if (isDownloading) {
        ShowStatus("Download already in progress...");
        return;
    }
    
    if (strlen(url) == 0) {
        ShowStatus("Please enter a YouTube URL");
        return;
    }
    
    isDownloading = true;
    ShowStatus("Starting download...");
    
    char command[1024];
    snprintf(command, sizeof(command), "python3 yt_py.py \"%s\" 2>/dev/null", url);
    
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        ShowStatus("Failed to start download process");
        isDownloading = false;
        return;
    }
    
    char result[512] = {0};
    char line[256];
    
    // Read all output from the process
    while (fgets(line, sizeof(line), pipe) != NULL) {
        // Only keep the last line (should be the filename or error)
        strncpy(result, line, sizeof(result) - 1);
        result[sizeof(result) - 1] = '\0';
    }
    
    int exit_status = pclose(pipe);
    
    // Remove newline if present
    char* newline = strchr(result, '\n');
    if (newline) *newline = '\0';
    
    if (exit_status == 0 && strlen(result) > 0 && 
        strstr(result, "Error:") == NULL && strstr(result, "ERROR:") == NULL) {
        // Successful download - update selected file path
        strncpy(selectedFilePath, result, sizeof(selectedFilePath) - 1);
        selectedFilePath[sizeof(selectedFilePath) - 1] = '\0';
        selectedMessageType = MSG_AUDIO;
        ShowStatus("Download complete! Audio ready to encode/send.");
    } else if (strlen(result) > 0) {
        ShowStatus(result);
    } else {
        ShowStatus("Download failed - check URL and internet connection");
    }
    
    isDownloading = false;
}
void GenerateRandomAudio() {
    TraceLog(LOG_INFO, "Generating random audio...");
    
    int sampleRate = 44100;
    int duration = 5; // 5 seconds
    int sampleCount = sampleRate * duration;
    
    Wave wave = { 0 };
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    
    short* samples = (short*)malloc(sampleCount * sizeof(short));
    if (!samples) {
        ShowStatus("Memory allocation failed for audio generation");
        return;
    }
    
    // Generate a simple sine wave with some noise
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float frequency = 440.0f + sin(t * 2.0f) * 100.0f; // Varying frequency
        float sample = sin(2.0f * PI * frequency * t) * 0.5f;
        sample += (GetRandomValue(-1000, 1000) / 10000.0f) * 0.1f; // Add some noise
        samples[i] = (short)(sample * 32767);
    }
    
    wave.data = samples;
    
    bool success = ExportWave(wave, "randomAudio.wav");
    UnloadWave(wave);
    
    if (success) {
        ShowStatus("Random audio generated: randomAudio.wav");
        // Auto-select the generated file
        strcpy(selectedFilePath, "randomAudio.wav");
        selectedMessageType = MSG_AUDIO;
    } else {
        ShowStatus("Failed to generate random audio");
    }
}

void GenerateRandomImage() {
    TraceLog(LOG_INFO, "Generating random image...");
    
    int width = 512;
    int height = 512;
    Image image = GenImageColor(width, height, WHITE);
    
    if (image.data == NULL) {
        ShowStatus("Failed to create image");
        return;
    }

    // Color every pixel with a random color
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color color = {
                GetRandomValue(0, 255),
                GetRandomValue(0, 255),
                GetRandomValue(0, 255),
                255
            };
            ImageDrawPixel(&image, x, y, color);
        }
    }

    bool success = ExportImage(image, "randomImage.png");
    UnloadImage(image);
    
    if (success) {
        ShowStatus("Random image generated: randomImage.png");
        // Auto-select the generated file
        strcpy(selectedFilePath, "randomImage.png");
        selectedMessageType = MSG_IMAGE;
    } else {
        ShowStatus("Failed to generate random image");
    }
}
void LoadImageForDisplay(const char* imagePath) {
    if (imageLoaded) {
        UnloadTexture(currentImageTexture);
        imageLoaded = false;
    }
    
    if (FileExists(imagePath)) {
        Image image = LoadImage(imagePath);
        if (image.data != NULL) {
            // Resize image if too large for display
            if (image.width > 400 || image.height > 300) {
                float scale = fminf(400.0f / image.width, 300.0f / image.height);
                int newWidth = (int)(image.width * scale);
                int newHeight = (int)(image.height * scale);
                ImageResize(&image, newWidth, newHeight);
            }
            currentImageTexture = LoadTextureFromImage(image);
            UnloadImage(image);
            imageLoaded = true;
        }

    // Cleanup textures
    if (imageLoaded) {
        UnloadTexture(currentImageTexture);
    }
    
    // Cleanup message images and audio
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (messageImageLoaded[i]) {
            UnloadTexture(messageImages[i]);
        }
        if (messageAudioLoaded[i]) {
            UnloadSound(messageAudioSounds[i]);
        }
    }
}
}

// Steganography functions (simplified and safer versions)
void EncodeMessageInImage(const char* imagePath, const char* message, const char* outputPath) {
    if (!FileExists(imagePath)) {
        ShowStatus("Image file not found");
        return;
    }
    
    Image image = LoadImage(imagePath);
    if (image.data == NULL) {
        ShowStatus("Failed to load image");
        return;
    }
    
    int messageLen = strlen(message);
    if (messageLen > 100) { // Limit message length for safety
        UnloadImage(image);
        ShowStatus("Hidden message too long");
        return;
    }
    
    // Ensure we have enough pixels
    if (image.width * image.height < (messageLen + 1) * 8 + 32) {
        UnloadImage(image);
        ShowStatus("Image too small for message");
        return;
    }
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (image.data == NULL) {
        UnloadImage(image);
        ShowStatus("Failed to format image");
        return;
    }
    
    Color* pixels = (Color*)image.data;
    int totalPixels = image.width * image.height;
    
    // Encode message length in first 32 bits
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 4;
        if (pixelIndex >= totalPixels) break;
        
        int colorComponent = i % 4;
        int bit = (messageLen >> (31 - i)) & 1;
        
        unsigned char* component = NULL;
        switch (colorComponent) {
            case 0: component = &pixels[pixelIndex].r; break;
            case 1: component = &pixels[pixelIndex].g; break;
            case 2: component = &pixels[pixelIndex].b; break;
            case 3: component = &pixels[pixelIndex].a; break;
        }
        if (component) *component = (*component & 0xFE) | bit;
    }
    
    // Encode message with null terminator
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int idx = 32 + i * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            int bitValue = (ch >> (7 - bit)) & 1;
            
            unsigned char* component = NULL;
            switch (colorComponent) {
                case 0: component = &pixels[pixelIndex].r; break;
                case 1: component = &pixels[pixelIndex].g; break;
                case 2: component = &pixels[pixelIndex].b; break;
                case 3: component = &pixels[pixelIndex].a; break;
            }
            if (component) *component = (*component & 0xFE) | bitValue;
        }
    }
    
    bool success = ExportImage(image, outputPath);
    UnloadImage(image);
    
    if (!success) {
        ShowStatus("Failed to save encoded image");
    }
}

char* DecodeMessageFromImage(const char* imagePath) {
    if (!FileExists(imagePath)) return NULL;
    
    Image image = LoadImage(imagePath);
    if (image.data == NULL) return NULL;
    
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    
    if (image.data == NULL) {
        UnloadImage(image);
        return NULL;
    }
    
    Color* pixels = (Color*)image.data;
    int totalPixels = image.width * image.height;
    
    // Decode message length
    int messageLen = 0;
    for (int i = 0; i < 32; i++) {
        int pixelIndex = i / 4;
        if (pixelIndex >= totalPixels) break;
        
        int colorComponent = i % 4;
        
        unsigned char component = 0;
        switch (colorComponent) {
            case 0: component = pixels[pixelIndex].r; break;
            case 1: component = pixels[pixelIndex].g; break;
            case 2: component = pixels[pixelIndex].b; break;
            case 3: component = pixels[pixelIndex].a; break;
        }
        
        int bit = component & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 100) { // Limit for safety
        UnloadImage(image);
        return NULL;
    }
    
    // Decode message
    char* decodedMessage = (char*)malloc(messageLen + 1);
    if (!decodedMessage) {
        UnloadImage(image);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int idx = 32 + i * 8 + bit;
            int pixelIndex = idx / 4;
            if (pixelIndex >= totalPixels) break;
            
            int colorComponent = idx % 4;
            
            unsigned char component = 0;
            switch (colorComponent) {
                case 0: component = pixels[pixelIndex].r; break;
                case 1: component = pixels[pixelIndex].g; break;
                case 2: component = pixels[pixelIndex].b; break;
                case 3: component = pixels[pixelIndex].a; break;
            }
            
            int bitValue = component & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadImage(image);
    return decodedMessage;
}

void EncodeMessageInAudio(const char* audioPath, const char* message, const char* outputPath) {
    if (!FileExists(audioPath)) {
        ShowStatus("Audio file not found");
        return;
    }
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) {
        ShowStatus("Failed to load audio file");
        return;
    }
    
    int messageLen = strlen(message);
    if (messageLen > 100) { // Limit for safety
        UnloadWave(wave);
        ShowStatus("Hidden message too long for audio");
        return;
    }
    
    // Ensure 16-bit format
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    if (wave.data == NULL) {
        UnloadWave(wave);
        ShowStatus("Failed to format audio");
        return;
    }
    
    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    // Check if we have enough samples
    if (totalSamples < (messageLen + 1) * 8 + 32) {
        UnloadWave(wave);
        ShowStatus("Audio file too short for message");
        return;
    }
    
    // Encode message length
    for (int i = 0; i < 32 && i < totalSamples; i++) {
        int bit = (messageLen >> (31 - i)) & 1;
        if (bit) samples[i] |= 1;
        else samples[i] &= ~1;
    }
    
    // Encode message
    for (int i = 0; i <= messageLen; i++) {
        char ch = (i == messageLen) ? '\0' : message[i];
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex >= totalSamples) break;
            
            int bitValue = (ch >> (7 - bit)) & 1;
            if (bitValue) samples[sampleIndex] |= 1;
            else samples[sampleIndex] &= ~1;
        }
    }
    
    bool success = ExportWave(wave, outputPath);
    UnloadWave(wave);
    
    if (!success) {
        ShowStatus("Failed to save encoded audio");
    }
}

char* DecodeMessageFromAudio(const char* audioPath) {
    if (!FileExists(audioPath)) return NULL;
    
    Wave wave = LoadWave(audioPath);
    if (wave.data == NULL) return NULL;
    
    if (wave.sampleSize != 16) {
        WaveFormat(&wave, wave.sampleRate, 16, wave.channels);
    }
    
    if (wave.data == NULL) {
        UnloadWave(wave);
        return NULL;
    }
    
    short* samples = (short*)wave.data;
    int totalSamples = wave.frameCount * wave.channels;
    
    // Decode message length
    int messageLen = 0;
    for (int i = 0; i < 32 && i < totalSamples; i++) {
        int bit = samples[i] & 1;
        messageLen |= (bit << (31 - i));
    }
    
    if (messageLen <= 0 || messageLen > 100) {
        UnloadWave(wave);
        return NULL;
    }
    
    // Decode message
    char* decodedMessage = (char*)malloc(messageLen + 1);
    if (!decodedMessage) {
        UnloadWave(wave);
        return NULL;
    }
    
    for (int i = 0; i < messageLen; i++) {
        char ch = 0;
        for (int bit = 0; bit < 8; bit++) {
            int sampleIndex = 32 + i * 8 + bit;
            if (sampleIndex >= totalSamples) break;
            
            int bitValue = samples[sampleIndex] & 1;
            ch |= (bitValue << (7 - bit));
        }
        decodedMessage[i] = ch;
    }
    decodedMessage[messageLen] = '\0';
    
    UnloadWave(wave);
    return decodedMessage;
}

// Network functions with better error handling
void InitializeConnection(bool asServer, const char* ip, int port) {
    if (connection.isConnected) {
        CloseConnection();
    }
    
    connection.localPort = port;
    strncpy(connection.localIP, asServer ? "0.0.0.0" : "127.0.0.1", sizeof(connection.localIP) - 1);
    connection.localIP[sizeof(connection.localIP) - 1] = '\0';
    
    if (asServer) {
        // Server mode
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            ShowStatus("Failed to create socket");
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            ShowStatus("Bind failed");
            close(server_fd);
            return;
        }
        
        if (listen(server_fd, 3) < 0) {
            ShowStatus("Listen failed");
            close(server_fd);
            return;
        }
        
        ShowStatus("Waiting for connection...");
        
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        memset(&client_addr, 0, sizeof(client_addr));
        
        connection.socket_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (connection.socket_fd < 0) {
            ShowStatus("Accept failed");
            close(server_fd);
            return;
        }
        
        strncpy(connection.remoteIP, inet_ntoa(client_addr.sin_addr), sizeof(connection.remoteIP) - 1);
        connection.remoteIP[sizeof(connection.remoteIP) - 1] = '\0';
        connection.remotePort = ntohs(client_addr.sin_port);
        close(server_fd);
    } else {
        // Client mode
        connection.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connection.socket_fd < 0) {
            ShowStatus("Failed to create socket");
            return;
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
            ShowStatus("Invalid address");
            close(connection.socket_fd);
            return;
        }
        
        if (connect(connection.socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            ShowStatus("Connection failed");
            close(connection.socket_fd);
            return;
        }
        
        strncpy(connection.remoteIP, ip, sizeof(connection.remoteIP) - 1);
        connection.remoteIP[sizeof(connection.remoteIP) - 1] = '\0';
        connection.remotePort = port;
    }
    
    connection.isConnected = true;
    connection.threadActive = true;
    showConnectionDialog = false;
    ShowStatus("Connected!");
    
    // Start receive thread
    int result = pthread_create(&connection.receiveThread, NULL, ReceiveMessages, NULL);
    if (result != 0) {
        ShowStatus("Failed to create receive thread");
        connection.isConnected = false;
        close(connection.socket_fd);
    }
}

void CloseConnection() {
    if (connection.isConnected) {
        connection.isConnected = false;
        connection.threadActive = false;
        
        // Shutdown socket to wake up blocking recv
        shutdown(connection.socket_fd, SHUT_RDWR);
        close(connection.socket_fd);
        
        // Wait for thread to finish (macOS compatible)
        pthread_join(connection.receiveThread, NULL);
    }
}

void SendMessage(const char* message, MessageType type) {
    if (!connection.isConnected || !message) return;
    
    // Create packet with proper alignment
    unsigned char* packet = (unsigned char*)malloc(BUFFER_SIZE);
    if (!packet) return;
    
    int offset = 0;
    
    // Add message type
    packet[offset++] = (unsigned char)type;
    
    // Add message length (ensure proper alignment)
    int msgLen = strlen(message);
    memcpy(packet + offset, &msgLen, sizeof(int));
    offset += sizeof(int);
    
    // Add message
    memcpy(packet + offset, message, msgLen);
    offset += msgLen;
    
    // Send packet
    ssize_t sent = send(connection.socket_fd, packet, offset, 0);
    free(packet);
    
    if (sent > 0) {
        AddMessage("You", message, type, true);
    } else {
        ShowStatus("Failed to send message");
    }
}

void SendFile(const char* filepath, MessageType type) {
    if (!connection.isConnected || !FileExists(filepath)) {
        ShowStatus("Cannot send file - not connected or file not found");
        return;
    }
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        ShowStatus("Failed to open file");
        return;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) { // 10MB limit
        fclose(file);
        ShowStatus("File too large or invalid");
        return;
    }
    
    // Read file
    unsigned char* fileData = (unsigned char*)malloc(fileSize);
    if (!fileData) {
        fclose(file);
        ShowStatus("Memory allocation failed");
        return;
    }
    
    size_t bytesRead = fread(fileData, 1, fileSize, file);
    fclose(file);
    
    if (bytesRead != fileSize) {
        free(fileData);
        ShowStatus("Failed to read file");
        return;
    }
    
    // Create temporary encoded file if hidden message exists
    char actualFilePath[256];
    strncpy(actualFilePath, filepath, sizeof(actualFilePath) - 1);
    actualFilePath[sizeof(actualFilePath) - 1] = '\0';
    
    if (strlen(hiddenMessageBuffer) > 0) {
        if (type == MSG_IMAGE) {
            sprintf(actualFilePath, "temp_encoded_%d.png", (int)time(NULL));
            EncodeMessageInImage(filepath, hiddenMessageBuffer, actualFilePath);
        } else if (type == MSG_AUDIO) {
            sprintf(actualFilePath, "temp_encoded_%d.wav", (int)time(NULL));
            EncodeMessageInAudio(filepath, hiddenMessageBuffer, actualFilePath);
        }
        
        // Reload the encoded file
        free(fileData);
        file = fopen(actualFilePath, "rb");
        if (!file) {
            ShowStatus("Failed to open encoded file");
            return;
        }
        
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        fileData = (unsigned char*)malloc(fileSize);
        if (!fileData) {
            fclose(file);
            ShowStatus("Memory allocation failed");
            return;
        }
        
        fread(fileData, 1, fileSize, file);
        fclose(file);
    }
    
    // Send file info packet
    unsigned char* packet = (unsigned char*)malloc(BUFFER_SIZE);
    if (!packet) {
        free(fileData);
        ShowStatus("Memory allocation failed");
        return;
    }
    
    int offset = 0;
    packet[offset++] = (unsigned char)type;
    
    // Add filename
    const char* filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;
    int nameLen = strlen(filename);
    
    memcpy(packet + offset, &nameLen, sizeof(int));
    offset += sizeof(int);
    memcpy(packet + offset, filename, nameLen);
    offset += nameLen;
    
    // Add file size
    memcpy(packet + offset, &fileSize, sizeof(long));
    offset += sizeof(long);
    
    // Send file info
    ssize_t sent = send(connection.socket_fd, packet, offset, 0);
    free(packet);
    
    if (sent <= 0) {
        free(fileData);
        ShowStatus("Failed to send file info");
        return;
    }
    
    // Send file data in chunks
    size_t sentBytes = 0;
    const size_t chunkSize = 8192;
    
    while (sentBytes < fileSize && connection.isConnected) {
        size_t remainingBytes = fileSize - sentBytes;
        size_t currentChunk = (remainingBytes > chunkSize) ? chunkSize : remainingBytes;
        
        sent = send(connection.socket_fd, fileData + sentBytes, currentChunk, 0);
        if (sent <= 0) {
            ShowStatus("Failed to send file data");
            break;
        }
        
        sentBytes += sent;
        usleep(1000); // Small delay between chunks
    }
    
    free(fileData);
    
    // Clean up temporary file
    if (strcmp(actualFilePath, filepath) != 0) {
        remove(actualFilePath);
    }
    
    if (sentBytes == fileSize) {
        char msg[512];
        sprintf(msg, "[%s] %s", type == MSG_IMAGE ? "Image" : "Audio", filename);
        AddMessage("You", msg, type, true);
        
        if (strlen(hiddenMessageBuffer) > 0) {
            ShowStatus("File sent with hidden message!");
            hiddenMessageBuffer[0] = '\0';
        } else {
            ShowStatus("File sent!");
        }
    }
}

void* ReceiveMessages(void* arg) {
    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);
    if (!buffer) return NULL;
    
    while (connection.isConnected && connection.threadActive) {
        int bytesReceived = recv(connection.socket_fd, buffer, BUFFER_SIZE, 0);
        
        if (bytesReceived <= 0) {
            if (connection.isConnected) {
                ShowStatus("Connection lost");
            }
            break;
        }
        
        int offset = 0;
        if (offset >= bytesReceived) continue;
        
        MessageType type = (MessageType)buffer[offset++];
        
        if (type == MSG_TEXT) {
            if (offset + sizeof(int) > bytesReceived) continue;
            
            int msgLen;
            memcpy(&msgLen, buffer + offset, sizeof(int));
            offset += sizeof(int);
            
            if (msgLen <= 0 || msgLen >= MAX_MESSAGE_LENGTH || offset + msgLen > bytesReceived) {
                continue;
            }
            
            char* message = (char*)malloc(msgLen + 1);
            if (!message) continue;
            
            memcpy(message, buffer + offset, msgLen);
            message[msgLen] = '\0';
            
            pthread_mutex_lock(&messageMutex);
            AddMessage("Contact", message, MSG_TEXT, false);
            pthread_mutex_unlock(&messageMutex);
            
            free(message);
            
        } else if (type == MSG_IMAGE || type == MSG_AUDIO) {
            if (offset + sizeof(int) > bytesReceived) continue;
            
            // Receive file info
            int nameLen;
            memcpy(&nameLen, buffer + offset, sizeof(int));
            offset += sizeof(int);
            
            if (nameLen <= 0 || nameLen >= 256 || offset + nameLen + sizeof(long) > bytesReceived) {
                continue;
            }
            
            char filename[256];
            memcpy(filename, buffer + offset, nameLen);
            filename[nameLen] = '\0';
            offset += nameLen;
            
            long fileSize;
            memcpy(&fileSize, buffer + offset, sizeof(long));
            
            if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) { // 10MB limit
                continue;
            }
            
            // Receive file data
            unsigned char* fileData = (unsigned char*)malloc(fileSize);
            if (!fileData) continue;
            
            size_t received = 0;
            while (received < fileSize && connection.isConnected) {
                int bytes = recv(connection.socket_fd, fileData + received, fileSize - received, 0);
                if (bytes <= 0) break;
                received += bytes;
            }
            
            if (received == fileSize) {
                // Save file
                char savePath[512];
                sprintf(savePath, "received_%s", filename);
                FILE* saveFile = fopen(savePath, "wb");
                if (saveFile) {
                    fwrite(fileData, 1, fileSize, saveFile);
                    fclose(saveFile);
                    
                    // Try to decode hidden message
                    char* hiddenMsg = NULL;
                    if (type == MSG_IMAGE) {
                        hiddenMsg = DecodeMessageFromImage(savePath);
                    } else if (type == MSG_AUDIO) {
                        hiddenMsg = DecodeMessageFromAudio(savePath);
                    }
                    
                    pthread_mutex_lock(&messageMutex);
                    char msg[512];
                    sprintf(msg, "[%s] %s", type == MSG_IMAGE ? "Image" : "Audio", filename);
                    AddMessage("Contact", msg, type, false);
                    
                    if (hiddenMsg && strlen(hiddenMsg) > 0) {
                        messages[messageCount - 1].hasHiddenMessage = true;
                        strncpy(messages[messageCount - 1].hiddenMessage, hiddenMsg, sizeof(messages[messageCount - 1].hiddenMessage) - 1);
                        messages[messageCount - 1].hiddenMessage[sizeof(messages[messageCount - 1].hiddenMessage) - 1] = '\0';
                        free(hiddenMsg);
                    }
                    pthread_mutex_unlock(&messageMutex);
                }
            }
            
            free(fileData);
        }
    }
    
    free(buffer);
    connection.isConnected = false;
    return NULL;
}

void AddMessage(const char* sender, const char* content, MessageType type, bool isSent) {
    if (!sender || !content) return;
    
    if (messageCount >= MAX_MESSAGES) {
        // Shift messages up
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            messages[i] = messages[i + 1];
        }
        messageCount--;
    }
    
    ChatMessage* msg = &messages[messageCount++];
    strncpy(msg->sender, sender, sizeof(msg->sender) - 1);
    msg->sender[sizeof(msg->sender) - 1] = '\0';
    strncpy(msg->content, content, sizeof(msg->content) - 1);
    msg->content[sizeof(msg->content) - 1] = '\0';
    msg->type = type;
    msg->isSent = isSent;
    msg->hasHiddenMessage = false;
    msg->hiddenMessage[0] = '\0';
    
    // Get timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info) {
        strftime(msg->timestamp, sizeof(msg->timestamp), "%H:%M", tm_info);
    } else {
        strcpy(msg->timestamp, "??:??");
    }
}

void ShowStatus(const char* message) {
    if (message) {
        strncpy(statusMessage, message, sizeof(statusMessage) - 1);
        statusMessage[sizeof(statusMessage) - 1] = '\0';
        statusTimer = 3.0f;
    }
}

void DrawConnectionDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 400;
    int dialogHeight = 300;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("StegaChat Connection", dialogX + 20, dialogY + 20, 20, DARKGRAY);
    
    // Server/Client toggle
    if (GuiButton((Rectangle){dialogX + 50, dialogY + 60, 120, 40}, isServer ? "Server Mode" : "Client Mode")) {
        isServer = !isServer;
    }
    
    if (!isServer) {
        DrawText("Server IP:", dialogX + 20, dialogY + 120, 16, DARKGRAY);
        if (GuiTextBox((Rectangle){dialogX + 100, dialogY + 115, 200, 30}, serverIPBuffer, 19, serverIPEditMode)) {
            serverIPEditMode = !serverIPEditMode;
        }
    }
    
    DrawText("Port:", dialogX + 20, dialogY + 160, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){dialogX + 100, dialogY + 155, 100, 30}, portBuffer, 9, portEditMode)) {
        portEditMode = !portEditMode;
    }
    
    // Connect button
    if (GuiButton((Rectangle){dialogX + 150, dialogY + 210, 100, 40}, "Connect")) {
        int port = atoi(portBuffer);
        if (port > 0 && port < 65536) {
            InitializeConnection(isServer, serverIPBuffer, port);
        } else {
            ShowStatus("Invalid port number");
        }
    }
    
    // Status
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        DrawText(statusMessage, dialogX + 20, dialogY + 260, 14, RED);
    }
}
void DrawDecodeDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 500;
    int dialogHeight = 400;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("Decode Hidden Message", dialogX + 20, dialogY + 20, 18, DARKGRAY);
    
    // If we're decoding from a specific message
    if (selectedMessageIndex >= 0 && selectedMessageIndex < messageCount) {
        ChatMessage* msg = &messages[selectedMessageIndex];
        
        // Show message info
        char msgInfo[256];
        sprintf(msgInfo, "Decoding from: %s", msg->content);
        DrawText(msgInfo, dialogX + 20, dialogY + 60, 14, DARKGRAY);
        
        // Show image if it's an image message
        if (msg->type == MSG_IMAGE && !msg->isSent) {
            char imagePath[512];
            sprintf(imagePath, "received_%s", strrchr(msg->content, ']') + 2);
            
            if (FileExists(imagePath)) {
                if (!imageLoaded) {
                    LoadImageForDisplay(imagePath);
                }
                
                if (imageLoaded) {
                    int imgX = dialogX + (dialogWidth - currentImageTexture.width) / 2;
                    int imgY = dialogY + 90;
                    DrawTexture(currentImageTexture, imgX, imgY, WHITE);
                }
            }
        }
        
        // Decode button for the selected message
        if (GuiButton((Rectangle){dialogX + 20, dialogY + 320, 100, 30}, "Decode This")) {
            char filePath[512];
            if (msg->type == MSG_IMAGE) {
                sprintf(filePath, "received_%s", strrchr(msg->content, ']') + 2);
                char* decoded = DecodeMessageFromImage(filePath);
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found");
                }
            } else if (msg->type == MSG_AUDIO) {
                sprintf(filePath, "received_%s", strrchr(msg->content, ']') + 2);
                char* decoded = DecodeMessageFromAudio(filePath);
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found");
                }
            }
        }
    } else {
        // Manual file decode
        DrawText("File path to decode:", dialogX + 20, dialogY + 60, 14, DARKGRAY);
        if (GuiTextBox((Rectangle){dialogX + 20, dialogY + 80, 350, 30}, decodeFilePath, 255, decodeFilePathEditMode)) {
            decodeFilePathEditMode = !decodeFilePathEditMode;
        }
        
        // Auto-detect file type and decode
        if (GuiButton((Rectangle){dialogX + 380, dialogY + 80, 80, 30}, "Decode")) {
            if (strlen(decodeFilePath) > 0 && FileExists(decodeFilePath)) {
                char* decoded = NULL;
                
                // Check file extension to determine type
                const char* ext = strrchr(decodeFilePath, '.');
                if (ext && (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)) {
                    decoded = DecodeMessageFromImage(decodeFilePath);
                    if (!imageLoaded) {
                        LoadImageForDisplay(decodeFilePath);
                    }
                } else if (ext && strcmp(ext, ".wav") == 0) {
                    decoded = DecodeMessageFromAudio(decodeFilePath);
                }
                
                if (decoded) {
                    strncpy(decodedHiddenMessage, decoded, sizeof(decodedHiddenMessage) - 1);
                    decodedHiddenMessage[sizeof(decodedHiddenMessage) - 1] = '\0';
                    free(decoded);
                    ShowStatus("Hidden message decoded!");
                } else {
                    strcpy(decodedHiddenMessage, "No hidden message found or unsupported format");
                }
            } else {
                ShowStatus("File not found!");
            }
        }
        
        // Show loaded image
        if (imageLoaded) {
            int imgX = dialogX + (dialogWidth - currentImageTexture.width) / 2;
            int imgY = dialogY + 120;
            DrawTexture(currentImageTexture, imgX, imgY, WHITE);
        }
    }
    
    // Show decoded message
    if (strlen(decodedHiddenMessage) > 0) {
        DrawText("Decoded message:", dialogX + 20, dialogY + 280, 14, (Color){255, 152, 0, 255});
        DrawRectangle(dialogX + 20, dialogY + 300, dialogWidth - 40, 40, (Color){255, 248, 225, 255});
        DrawRectangleLines(dialogX + 20, dialogY + 300, dialogWidth - 40, 40, (Color){255, 193, 7, 255});
        DrawText(decodedHiddenMessage, dialogX + 25, dialogY + 315, 12, DARKGRAY);
    }
    
    // Close button
    if (GuiButton((Rectangle){dialogX + dialogWidth - 80, dialogY + 360, 60, 25}, "Close")) {
        showDecodeDialog = false;
        selectedMessageIndex = -1;
        decodedHiddenMessage[0] = '\0';
        decodeFilePath[0] = '\0';
        if (imageLoaded) {
            UnloadTexture(currentImageTexture);
            imageLoaded = false;
        }
    }
}

void DrawYTDialog() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 180});
    
    // Dialog box
    int dialogWidth = 500;
    int dialogHeight = 300;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    DrawRectangle(dialogX, dialogY, dialogWidth, dialogHeight, WHITE);
    DrawRectangleLines(dialogX, dialogY, dialogWidth, dialogHeight, DARKGRAY);
    
    // Title
    DrawText("YouTube Audio Download", dialogX + 20, dialogY + 20, 18, DARKGRAY);
    
    // Instructions
    DrawText("Enter YouTube URL suffix (we'll add https://www.youtube.com/):", dialogX + 20, dialogY + 60, 14, GRAY);
    
    // URL input with prefix
    DrawText("https://www.youtube.com/", dialogX + 20, dialogY + 90, 12, DARKGRAY);
    int prefixWidth = MeasureText("https://www.youtube.com/", 12);
    if (GuiTextBox((Rectangle){dialogX + 20 + prefixWidth, dialogY + 85, 350 - prefixWidth, 30}, 
                   ytUrlBuffer + 25, 487, ytUrlEditMode)) { // +25 to skip the prefix
        ytUrlEditMode = !ytUrlEditMode;
    }
    
    // Download button
    const char* downloadButtonText = isDownloading ? "Downloading..." : "Download";
    Color downloadButtonColor = isDownloading ? LIGHTGRAY : (Color){76, 175, 80, 255};
    
    if (GuiButton((Rectangle){dialogX + 420, dialogY + 85, 70, 30}, downloadButtonText)) {
        if (!isDownloading && strlen(ytUrlBuffer) > 0) {
            DownloadYouTubeAudio(ytUrlBuffer);
        }
    }
    
    // Instructions text
    DrawText("Downloaded audio will be automatically selected for encoding/sending.", 
             dialogX + 20, dialogY + 140, 12, GRAY);
    DrawText("Make sure yt_py.py is in the same directory.", 
             dialogX + 20, dialogY + 160, 12, GRAY);
    
    // Status area
    if (strlen(statusMessage) > 0 && statusTimer > 0) {
        DrawRectangle(dialogX + 20, dialogY + 190, dialogWidth - 40, 60, (Color){248, 249, 250, 255});
        DrawRectangleLines(dialogX + 20, dialogY + 190, dialogWidth - 40, 60, LIGHTGRAY);
        DrawText("Status:", dialogX + 30, dialogY + 200, 12, DARKGRAY);
        
        // Word wrap for status message
        const char* status = statusMessage;
        int maxWidth = dialogWidth - 60;
        int lineY = dialogY + 220;
        
        // Simple text wrapping for status
        if (MeasureText(status, 12) > maxWidth) {
            char line1[128], line2[128];
            int splitPoint = strlen(status) / 2;
            // Find nearest space
            while (splitPoint > 0 && status[splitPoint] != ' ') splitPoint--;
            if (splitPoint == 0) splitPoint = strlen(status) / 2;
            
            strncpy(line1, status, splitPoint);
            line1[splitPoint] = '\0';
            strcpy(line2, status + splitPoint + 1);
            
            DrawText(line1, dialogX + 30, lineY, 12, (Color){76, 175, 80, 255});
            DrawText(line2, dialogX + 30, lineY + 15, 12, (Color){76, 175, 80, 255});
        } else {
            DrawText(status, dialogX + 30, lineY, 12, (Color){76, 175, 80, 255});
        }
    }
    
    // Close button
    if (GuiButton((Rectangle){dialogX + dialogWidth - 80, dialogY + 260, 60, 25}, "Close")) {
        showYTDialog = false;
        ytUrlBuffer[0] = '\0';
    }
}

void DrawChat() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Soft gradient background
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, 
                           (Color){229, 221, 213, 255}, 
                           (Color){219, 209, 199, 255});
    
    // Refined header with depth
    DrawRectangle(0, 0, screenWidth, 70, (Color){7, 94, 84, 255});
    DrawRectangleLinesEx((Rectangle){0, 0, screenWidth, 70}, 2, (Color){200, 200, 200, 255});
    
    // Softer contact info
    DrawCircle(50, 35, 22, (Color){63, 81, 181, 255});
    DrawText("C", 44, 27, 22, WHITE);
    DrawText(connection.isConnected ? "Contact" : "Not Connected", 80, 20, 20, WHITE);
    DrawText(connection.isConnected ? "Online" : "Offline", 80, 45, 16, (Color){0, 188, 212, 255});
    
    // More refined message bubbles
    pthread_mutex_lock(&messageMutex);
    for (int i = 0; i < messageCount; i++) {
        ChatMessage* msg = &messages[i];
        
        int msgWidth = 350;
        int msgX = msg->isSent ? screenWidth - msgWidth - 20 : 20;
        
        // Calculate message height based on type
        int msgHeight = 60;
        if (msg->type == MSG_IMAGE || msg->type == MSG_AUDIO) {
            msgHeight = 120; // Larger for media messages
        }
        
        // Skip if outside visible area
        if (messageY + msgHeight < chatAreaY || messageY > chatAreaY + chatAreaHeight) {
            messageY += msgHeight + 10;
            continue;
        }
        
        // Message bubble
        Color bubbleColor = msg->isSent ? 
            (Color){240, 248, 255, 255} :  // Soft Blue for sent
            (Color){255, 255, 255, 255};   // Pure White for received
        
        // Subtle shadow effect
        DrawRectangleShadow((Rectangle){msgX, messageY, msgWidth, msgHeight}, 0.1f, 8, 
                            bubbleColor, (Color){200, 200, 200, 100});
        
        // More refined text rendering
        DrawTextEx(GetFontDefault(), msg->content, 
                   (Vector2){msgX + 10, messageY + 10}, 16, 1, (Color){33, 33, 33, 255});
    }
    pthread_mutex_unlock(&messageMutex);
    
    // Handle scrolling
    float wheel = GetMouseWheelMove();
    scrollOffset -= wheel * 40;
    if (scrollOffset < 0) scrollOffset = 0;
    
    // Enhanced input area with subtle border
    int inputAreaY = screenHeight - 150;
    DrawRectangle(0, inputAreaY, screenWidth, 150, (Color){240, 240, 240, 255});
    DrawRectangleLinesEx((Rectangle){0, inputAreaY, screenWidth, 150}, 1, (Color){200, 200, 200, 255});
    
    // Tool buttons row
    int buttonY = inputAreaY + 10;
    int buttonWidth = 70;
    int buttonSpacing = 80;
    int startX = 20;
    
    // Generate Random Image button
    if (GuiButton((Rectangle){startX, buttonY, buttonWidth, 25}, "Gen Image")) {
        GenerateRandomImage();
    }
    
    // Generate Random Audio button
    if (GuiButton((Rectangle){startX + buttonSpacing, buttonY, buttonWidth, 25}, "Gen Audio")) {
        GenerateRandomAudio();
    }
    
    // YouTube Download button
    if (GuiButton((Rectangle){startX + buttonSpacing * 2, buttonY, buttonWidth, 25}, "YT Download")) {
        showYTDialog = true;
    }
    
    // Encode button
    if (GuiButton((Rectangle){startX + buttonSpacing * 3, buttonY, buttonWidth, 25}, "Encode")) {
        if (strlen(selectedFilePath) > 0 && strlen(hiddenMessageBuffer) > 0) {
            char outputPath[512];
            sprintf(outputPath, "encoded_%d_%s", (int)time(NULL), 
                   strrchr(selectedFilePath, '/') ? strrchr(selectedFilePath, '/') + 1 : selectedFilePath);
            
            if (selectedMessageType == MSG_IMAGE) {
                EncodeMessageInImage(selectedFilePath, hiddenMessageBuffer, outputPath);
                strcpy(selectedFilePath, outputPath);
                ShowStatus("Image encoded! Ready to send.");
            } else if (selectedMessageType == MSG_AUDIO) {
                EncodeMessageInAudio(selectedFilePath, hiddenMessageBuffer, outputPath);
                strcpy(selectedFilePath, outputPath);
                ShowStatus("Audio encoded! Ready to send.");
            }
        } else {
            ShowStatus("Need file path and hidden message to encode");
        }
    }
    
    // Send Encoded button (separate from regular send)
    if (GuiButton((Rectangle){startX + buttonSpacing * 4, buttonY, buttonWidth, 25}, "Send Encoded")) {
        if (connection.isConnected && strlen(selectedFilePath) > 0 && FileExists(selectedFilePath)) {
            if (selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) {
                SendFile(selectedFilePath, selectedMessageType);
                selectedFilePath[0] = '\0';
                hiddenMessageBuffer[0] = '\0';
                ShowStatus("Encoded file sent!");
            }
        } else if (!connection.isConnected) {
            ShowStatus("Not connected!");
        } else {
            ShowStatus("No encoded file to send!");
        }
    }
    
    // Decode button
    if (GuiButton((Rectangle){startX + buttonSpacing * 5, buttonY, buttonWidth, 25}, "Decode File")) {
        showDecodeDialog = true;
        selectedMessageIndex = -1; // For manual decode
    }
    
    // Message type selector
    buttonY += 35;
    DrawText("Type:", startX, buttonY + 5, 14, DARKGRAY);
    
    if (GuiButton((Rectangle){startX + 50, buttonY, 60, 25}, selectedMessageType == MSG_TEXT ? "TEXT" : "Text")) {
        selectedMessageType = MSG_TEXT;
    }
    if (GuiButton((Rectangle){startX + 120, buttonY, 60, 25}, selectedMessageType == MSG_IMAGE ? "IMAGE" : "Image")) {
        selectedMessageType = MSG_IMAGE;
    }
    if (GuiButton((Rectangle){startX + 190, buttonY, 60, 25}, selectedMessageType == MSG_AUDIO ? "AUDIO" : "Audio")) {
        selectedMessageType = MSG_AUDIO;
    }
    
    // File path input for media
    if (selectedMessageType != MSG_TEXT) {
        buttonY += 35;
        DrawText("File:", startX, buttonY + 5, 12, DARKGRAY);
        if (GuiTextBox((Rectangle){startX + 40, buttonY, 400, 25}, selectedFilePath, 255, filePathEditMode)) {
            filePathEditMode = !filePathEditMode;
        }
        
        // Browse button
        if (GuiButton((Rectangle){startX + 450, buttonY, 60, 25}, "Browse")) {
            ShowStatus("Use file picker or type path manually");
        }
    }
    
    // Hidden message input (for steganography)
    if (selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) {
        buttonY += 35;
        DrawText("Hidden:", startX, buttonY + 5, 12, GRAY);
        if (GuiTextBox((Rectangle){startX + 60, buttonY, screenWidth - 200, 25}, hiddenMessageBuffer, MAX_MESSAGE_LENGTH - 1, hiddenMessageEditMode)) {
            hiddenMessageEditMode = !hiddenMessageEditMode;
        }
    }
    
    // Main message input
    buttonY += 35;
    DrawText("Message:", startX, buttonY + 5, 14, DARKGRAY);
    if (GuiTextBox((Rectangle){startX + 70, buttonY, screenWidth - 180, 25}, inputBuffer, MAX_MESSAGE_LENGTH - 1, inputEditMode)) {
        inputEditMode = !inputEditMode;
    }
    
    // Send button
    bool canSend = false;
    if (selectedMessageType == MSG_TEXT && strlen(inputBuffer) > 0) {
        canSend = true;
    } else if ((selectedMessageType == MSG_IMAGE || selectedMessageType == MSG_AUDIO) && 
               strlen(selectedFilePath) > 0 && FileExists(selectedFilePath)) {
        canSend = true;
    }
    
    Color sendButtonColor = canSend ? (Color){76, 175, 80, 255} : LIGHTGRAY;
    if (GuiButton((Rectangle){screenWidth - 90, buttonY, 70, 25}, "Send")) {
        if (canSend && connection.isConnected) {
            if (selectedMessageType == MSG_TEXT) {
                SendMessage(inputBuffer, MSG_TEXT);
                inputBuffer[0] = '\0';
            } else {
                SendFile(selectedFilePath, selectedMessageType);
                selectedFilePath[0] = '\0';
                hiddenMessageBuffer[0] = '\0'; // Clear hidden message after sending
            }
        } else if (!connection.isConnected) {
            ShowStatus("Not connected!");
        }
    }
    
    // Status message
    if (statusTimer > 0) {
        int statusX = screenWidth / 2 - MeasureText(statusMessage, 14) / 2;
        DrawText(statusMessage, statusX, inputAreaY - 25, (Color){76, 175, 80, 255});
    }
    
    // Decode dialog
    if (showDecodeDialog) {
        DrawDecodeDialog();
    }
    
    // YouTube download dialog
    if (showYTDialog) {
        DrawYTDialog();
    }
}

// Optional: Add a shadow rendering function
void DrawRectangleShadow(Rectangle rect, float roundness, int segments, 
                          Color fillColor, Color shadowColor) {
    // Draw subtle shadow first
    DrawRectangleRounded((Rectangle){
        rect.x + 2, 
        rect.y + 2, 
        rect.width, 
        rect.height
    }, roundness, segments, shadowColor);
    
    // Draw main rectangle
    DrawRectangleRounded(rect, roundness, segments, fillColor);
}

int main(void) {
    // Initialize window with proper flags
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(800, 600, "StegaChat - Secure Messaging with Steganography");
    
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    
    SetTargetFPS(60);
    
    // Initialize global variables safely
    memset(messages, 0, sizeof(messages));
    memset(&connection, 0, sizeof(connection));
    connection.isConnected = false;
    connection.threadActive = false;
    connection.socket_fd = -1;
    
    // Initialize buffers
    inputBuffer[0] = '\0';
    hiddenMessageBuffer[0] = '\0';
    selectedFilePath[0] = '\0';
    statusMessage[0] = '\0';
    
    while (!WindowShouldClose() && !shouldExit) {
        // Update status timer
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
        }
        
        // Handle Enter key for sending messages
        if (IsKeyPressed(KEY_ENTER) && !showConnectionDialog && connection.isConnected && !inputEditMode) {
            if (selectedMessageType == MSG_TEXT && strlen(inputBuffer) > 0) {
                SendMessage(inputBuffer, MSG_TEXT);
                inputBuffer[0] = '\0';
            }
        }
        
        // Handle Escape key
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (showEncodingOptions) {
                showEncodingOptions = false;
            } else if (!showConnectionDialog && connection.isConnected) {
                CloseConnection();
                showConnectionDialog = true;
            }
        }
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        if (showConnectionDialog) {
            DrawConnectionDialog();
        } else if (connection.isConnected || messageCount > 0) {
            DrawChat();
        } else {
            showConnectionDialog = true;
        }
        
        EndDrawing();
    }
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (messageImageLoaded[i]) {
            UnloadTexture(messageImages[i]);
        }
        if (messageAudioLoaded[i]) {
            UnloadSound(messageAudioSounds[i]);
        }
    }
    // Cleanup
    CloseConnection();
    
    // Clean up mutex
    pthread_mutex_destroy(&messageMutex);
    if (imageLoaded) {
        UnloadTexture(currentImageTexture);
    }
    if (IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    
    CloseWindow();
    
    return 0;
}