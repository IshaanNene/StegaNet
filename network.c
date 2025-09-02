#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"
#include "network.h"
#include "utils.h"       
#include "steganography.h"  

void InitializeConnection(bool asServer, const char* ip, int port) {
    if (connection.isConnected) {
        CloseConnection();
    }
    connection.localPort = port;
    strncpy(connection.localIP, asServer ? "0.0.0.0" : "127.0.0.1", sizeof(connection.localIP) - 1);
    connection.localIP[sizeof(connection.localIP) - 1] = '\0';
    if (asServer) {
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
        shutdown(connection.socket_fd, SHUT_RDWR);
        close(connection.socket_fd);
        pthread_join(connection.receiveThread, NULL);
    }
}

void SendMessage(const char* message, MessageType type) {
    if (!connection.isConnected || !message) return;
    unsigned char* packet = (unsigned char*)malloc(BUFFER_SIZE);
    if (!packet) return;
    int offset = 0;
    packet[offset++] = (unsigned char)type;
    int msgLen = strlen(message);
    memcpy(packet + offset, &msgLen, sizeof(int));
    offset += sizeof(int);
    memcpy(packet + offset, message, msgLen);
    offset += msgLen;
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
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) { 
        fclose(file);
        ShowStatus("File too large or invalid");
        return;
    }
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
    unsigned char* packet = (unsigned char*)malloc(BUFFER_SIZE);
    if (!packet) {
        free(fileData);
        ShowStatus("Memory allocation failed");
        return;
    }
    
    int offset = 0;
    packet[offset++] = (unsigned char)type;
    const char* filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;
    int nameLen = strlen(filename);
    
    memcpy(packet + offset, &nameLen, sizeof(int));
    offset += sizeof(int);
    memcpy(packet + offset, filename, nameLen);
    offset += nameLen;
    memcpy(packet + offset, &fileSize, sizeof(long));
    offset += sizeof(long);
    ssize_t sent = send(connection.socket_fd, packet, offset, 0);
    free(packet);
    
    if (sent <= 0) {
        free(fileData);
        ShowStatus("Failed to send file info");
        return;
    }
    
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
        usleep(1000); 
    }
    
    free(fileData);
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
            
            if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) { 
                continue;
            }
            unsigned char* fileData = (unsigned char*)malloc(fileSize);
            if (!fileData) continue;
            
            size_t received = 0;
            while (received < fileSize && connection.isConnected) {
                int bytes = recv(connection.socket_fd, fileData + received, fileSize - received, 0);
                if (bytes <= 0) break;
                received += bytes;
            }
            
            if (received == fileSize) {
                char savePath[512];
                sprintf(savePath, "received_%s", filename);
                FILE* saveFile = fopen(savePath, "wb");
                if (saveFile) {
                    fwrite(fileData, 1, fileSize, saveFile);
                    fclose(saveFile);
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
