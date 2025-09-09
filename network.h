#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

void InitializeConnection(bool asServer, const char* ip, int port);
void CloseConnection();
void SendMessage(const char* message, MessageType type);
void SendFile(const char* filepath, MessageType type);
void* ReceiveMessages(void* arg);

#endif
