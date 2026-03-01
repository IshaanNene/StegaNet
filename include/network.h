#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"

void InitializeConnection(AppState *state, bool asServer, const char *ip,
                          int port);
void CloseConnection(AppState *state);
void SendMessage(AppState *state, const char *message, MessageType type);
void SendFile(AppState *state, const char *filepath, MessageType type);
void *ReceiveMessages(void *arg);

#endif

void SendPing(AppState *state);
