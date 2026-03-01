import re
import os

filepath = "src/network.c"
with open(filepath, 'r') as f:
    content = f.read()

new_content = """#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "network.h"
#include "steganography.h"
#include "utils.h"

// Helper to send exactly N bytes
static bool SendAll(int socket, const unsigned char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(socket, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

// Helper to receive exactly N bytes
static bool RecvAll(int socket, unsigned char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = recv(socket, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

static void SendFramedMessage(AppState *state, MessageType type, const unsigned char *payload, size_t payloadLen) {
    if (!state->connection.isConnected) return;
    uint32_t totalLen = 1 + payloadLen; // 1 byte for type
    uint32_t netLen = htonl(totalLen);
    
    // Send length prefix (4 bytes)
    if (!SendAll(state->connection.socket_fd, (const unsigned char*)&netLen, 4)) return;
    
    // Send type (1 byte)
    unsigned char typeByte = (unsigned char)type;
    if (!SendAll(state->connection.socket_fd, &typeByte, 1)) return;
    
    // Send payload
    if (payloadLen > 0) {
        SendAll(state->connection.socket_fd, payload, payloadLen);
    }
}

void InitializeConnection(AppState *state, bool asServer, const char *ip,
                          int port) {
  if (state->connection.isConnected) {
    CloseConnection(state);
  }
  state->connection.localPort = port;
  strncpy(state->connection.localIP, asServer ? "0.0.0.0" : "127.0.0.1",
          sizeof(state->connection.localIP) - 1);
  state->connection.localIP[sizeof(state->connection.localIP) - 1] = '\\0';
  if (asServer) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
      ShowStatus(state, "Failed to create socket");
      return;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      ShowStatus(state, "Bind failed");
      close(server_fd);
      return;
    }

    if (listen(server_fd, 3) < 0) {
      ShowStatus(state, "Listen failed");
      close(server_fd);
      return;
    }
    ShowStatus(state, "Waiting for connection...");
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    state->connection.socket_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (state->connection.socket_fd < 0) {
      ShowStatus(state, "Accept failed");
      close(server_fd);
      return;
    }
    strncpy(state->connection.remoteIP, inet_ntoa(client_addr.sin_addr),
            sizeof(state->connection.remoteIP) - 1);
    state->connection.remoteIP[sizeof(state->connection.remoteIP) - 1] = '\\0';
    state->connection.remotePort = ntohs(client_addr.sin_port);
    close(server_fd);
  } else {
    state->connection.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->connection.socket_fd < 0) {
      ShowStatus(state, "Failed to create socket");
      return;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
      ShowStatus(state, "Invalid address");
      close(state->connection.socket_fd);
      return;
    }

    // Connect with simple timeout struct if needed, but standard block is ok for now.
    struct timeval tv;
    tv.tv_sec = 5; // 5 second timeout on recv
    tv.tv_usec = 0;
    
    if (connect(state->connection.socket_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
      ShowStatus(state, "Connection failed");
      close(state->connection.socket_fd);
      return;
    }
    
    setsockopt(state->connection.socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    strncpy(state->connection.remoteIP, ip, sizeof(state->connection.remoteIP) - 1);
    state->connection.remoteIP[sizeof(state->connection.remoteIP) - 1] = '\\0';
    state->connection.remotePort = port;
  }

  // Set timeout for both
  struct timeval tv_recv;
  tv_recv.tv_sec = 2; // 2 seconds recv timeout allowing thread loop
  tv_recv.tv_usec = 0;
  setsockopt(state->connection.socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_recv, sizeof(tv_recv));

  state->connection.isConnected = true;
  state->connection.threadActive = true;
  state->connection.lastPingSent = GetTime();
  state->connection.lastPongReceived = GetTime();
  state->connection.latency = 0;
  state->showConnectionDialog = false;
  ShowStatus(state, "Connected!");
  int result =
      pthread_create(&state->connection.receiveThread, NULL, ReceiveMessages, state);
  if (result != 0) {
    ShowStatus(state, "Failed to create receive thread");
    state->connection.isConnected = false;
    close(state->connection.socket_fd);
  }
}

void CloseConnection(AppState *state) {
  if (state->connection.isConnected) {
    state->connection.isConnected = false;
    state->connection.threadActive = false;
    shutdown(state->connection.socket_fd, SHUT_RDWR);
    close(state->connection.socket_fd);
    pthread_join(state->connection.receiveThread, NULL);
  }
}

void SendMessage(AppState *state, const char *message, MessageType type) {
  if (!state->connection.isConnected || !message)
    return;
    
  uint32_t msgLen = strlen(message);
  uint32_t netMsgLen = htonl(msgLen);
  
  size_t payloadLen = sizeof(uint32_t) + msgLen;
  unsigned char *payload = (unsigned char *)malloc(payloadLen);
  if (!payload) return;
  
  memcpy(payload, &netMsgLen, sizeof(uint32_t));
  memcpy(payload + sizeof(uint32_t), message, msgLen);
  
  SendFramedMessage(state, type, payload, payloadLen);
  free(payload);
  
  AddMessage(state, "You", message, type, true);
}

void SendFile(AppState *state, const char *filepath, MessageType type) {
  if (!state->connection.isConnected || !FileExists(filepath)) {
    ShowStatus(state, "Cannot send file - not connected or file not found");
    return;
  }
  
  char actualFilePath[256];
  strncpy(actualFilePath, filepath, sizeof(actualFilePath) - 1);
  actualFilePath[sizeof(actualFilePath) - 1] = '\\0';

  if (strlen(state->hiddenMessageBuffer) > 0) {
    if (type == MSG_IMAGE) {
      sprintf(actualFilePath, "temp_encoded_%d.png", (int)time(NULL));
      EncodeMessageInImage(state, filepath, state->hiddenMessageBuffer,
                           actualFilePath);
    } else if (type == MSG_AUDIO) {
      sprintf(actualFilePath, "temp_encoded_%d.wav", (int)time(NULL));
      EncodeMessageInAudio(state, filepath, state->hiddenMessageBuffer,
                           actualFilePath);
    }
  }

  FILE *file = fopen(actualFilePath, "rb");
  if (!file) {
    ShowStatus(state, "Failed to open file to send");
    return;
  }
  
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileSize <= 0 || fileSize > 10 * 1024 * 1024) {
    fclose(file);
    ShowStatus(state, "File too large or invalid");
    return;
  }
  
  unsigned char *fileData = (unsigned char *)malloc(fileSize);
  if (!fileData) {
    fclose(file);
    ShowStatus(state, "Memory allocation failed");
    return;
  }
  
  size_t bytesRead = fread(fileData, 1, fileSize, file);
  fclose(file);
  if (bytesRead != (size_t)fileSize) {
    free(fileData);
    ShowStatus(state, "Failed to read file");
    return;
  }
  
  const char *filename = strrchr(filepath, '/');
  filename = filename ? filename + 1 : filepath;
  uint32_t nameLen = strlen(filename);
  
  uint32_t netNameLen = htonl(nameLen);
  uint32_t netFileSize = htonl((uint32_t)fileSize); // Assuming < 4GB
  
  size_t payloadLen = sizeof(uint32_t) + nameLen + sizeof(uint32_t) + fileSize;
  unsigned char *payload = (unsigned char *)malloc(payloadLen);
  if (!payload) {
      free(fileData);
      ShowStatus(state, "Memory allocation failed");
      return;
  }
  
  size_t offset = 0;
  memcpy(payload + offset, &netNameLen, sizeof(uint32_t)); offset += sizeof(uint32_t);
  memcpy(payload + offset, filename, nameLen); offset += nameLen;
  memcpy(payload + offset, &netFileSize, sizeof(uint32_t)); offset += sizeof(uint32_t);
  memcpy(payload + offset, fileData, fileSize); offset += fileSize;
  
  SendFramedMessage(state, type, payload, payloadLen);
  
  free(payload);
  free(fileData);
  
  if (strcmp(actualFilePath, filepath) != 0) {
    remove(actualFilePath);
  }
  
  char msg[512];
  sprintf(msg, "[%s] %s", type == MSG_IMAGE ? "Image" : "Audio", filename);
  AddMessage(state, "You", msg, type, true);

  if (strlen(state->hiddenMessageBuffer) > 0) {
    ShowStatus(state, "File sent with hidden message!");
    state->hiddenMessageBuffer[0] = '\\0';
  } else {
    ShowStatus(state, "File sent!");
  }
}

void *ReceiveMessages(void *arg) {
  AppState *state = (AppState *)arg;

  while (state->connection.isConnected && state->connection.threadActive) {
    uint32_t netLen = 0;
    
    // Attempt to receive 4-byte length prefix
    ssize_t lenRead = recv(state->connection.socket_fd, &netLen, 4, MSG_WAITALL);
    if (lenRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            continue; // Timeout, loop again
        }
        if (state->connection.isConnected) ShowStatus(state, "Connection lost");
        break;
    } else if (lenRead == 0) {
        if (state->connection.isConnected) ShowStatus(state, "Connection closed by peer");
        break;
    } else if (lenRead != 4) {
        // Incomplete read of length header
        break;
    }
    
    uint32_t totalLen = ntohl(netLen);
    if (totalLen == 0 || totalLen > 15 * 1024 * 1024) { // Absolute max 15MB 
        // Invalid or overly large msg
        break;
    }
    
    unsigned char *payload = (unsigned char *)malloc(totalLen);
    if (!payload) break;
    
    if (!RecvAll(state->connection.socket_fd, payload, totalLen)) {
        free(payload);
        break;
    }
    
    MessageType type = (MessageType)payload[0];
    size_t payloadBytes = totalLen - 1;
    unsigned char *data = payload + 1;
    
    if (type == MSG_PING) {
        SendFramedMessage(state, MSG_PONG, NULL, 0);
    } else if (type == MSG_PONG) {
        state->connection.lastPongReceived = GetTime();
        state->connection.latency = (state->connection.lastPongReceived - state->connection.lastPingSent) * 1000.0f; // in ms
    } else if (type == MSG_TEXT) {
        if (payloadBytes >= 4) {
            uint32_t netStrLen;
            memcpy(&netStrLen, data, 4);
            uint32_t strLen = ntohl(netStrLen);
            
            if (strLen > 0 && strLen <= MAX_MESSAGE_LENGTH && payloadBytes >= 4 + strLen) {
                char *message = (char *)malloc(strLen + 1);
                memcpy(message, data + 4, strLen);
                message[strLen] = '\\0';
                
                pthread_mutex_lock(&state->messageMutex);
                AddMessage(state, "Contact", message, MSG_TEXT, false);
                pthread_mutex_unlock(&state->messageMutex);
                free(message);
            }
        }
    } else if (type == MSG_IMAGE || type == MSG_AUDIO) {
        if (payloadBytes >= 8) {
            uint32_t netNameLen;
            memcpy(&netNameLen, data, 4);
            uint32_t nameLen = ntohl(netNameLen);
            
            if (payloadBytes >= 4 + nameLen + 4) {
                char filename[256] = {0};
                size_t cpyLen = (nameLen < 255) ? nameLen : 255;
                memcpy(filename, data + 4, cpyLen);
                
                uint32_t netFileSize;
                memcpy(&netFileSize, data + 4 + nameLen, 4);
                uint32_t fileSize = ntohl(netFileSize);
                
                if (payloadBytes >= 4 + nameLen + 4 + fileSize && fileSize <= 10 * 1024 * 1024) {
                    char savePath[512];
                    sprintf(savePath, "received_%s", filename);
                    FILE *saveFile = fopen(savePath, "wb");
                    if (saveFile) {
                        fwrite(data + 4 + nameLen + 4, 1, fileSize, saveFile);
                        fclose(saveFile);
                        
                        char *hiddenMsg = NULL;
                        if (type == MSG_IMAGE) {
                            hiddenMsg = DecodeMessageFromImage(state, savePath);
                        } else if (type == MSG_AUDIO) {
                            hiddenMsg = DecodeMessageFromAudio(state, savePath);
                        }
                        
                        pthread_mutex_lock(&state->messageMutex);
                        char msg[512];
                        sprintf(msg, "[%s] %s", type == MSG_IMAGE ? "Image" : "Audio",
                                filename);
                        AddMessage(state, "Contact", msg, type, false);
              
                        if (hiddenMsg && strlen(hiddenMsg) > 0) {
                          state->messages[state->messageCount - 1].hasHiddenMessage = true;
                          strncpy(
                              state->messages[state->messageCount - 1].hiddenMessage,
                              hiddenMsg,
                              sizeof(state->messages[state->messageCount - 1].hiddenMessage) -
                                  1);
                          state->messages[state->messageCount - 1]
                              .hiddenMessage[sizeof(state->messages[state->messageCount - 1]
                                                        .hiddenMessage) -
                                             1] = '\\0';
                          free(hiddenMsg);
                        }
                        pthread_mutex_unlock(&state->messageMutex);
                    }
                }
            }
        }
    }
    
    free(payload);
  }

  state->connection.isConnected = false;
  return NULL;
}
"""

with open(filepath, 'w') as f:
    f.write(new_content)

