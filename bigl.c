#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// ---------------- Enums & Structs ----------------
typedef enum {
    STATE_MAIN_MENU,
    STATE_IMAGE_STEGO,
    STATE_AUDIO_STEGO,
    STATE_CHAT_INTERFACE
} AppState;

typedef enum {
    MSG_TEXT,
    MSG_IMAGE,
    MSG_AUDIO
} MessageType;

typedef struct {
    MessageType type;
    char sender_ip[INET_ADDRSTRLEN];
    char content[512]; // for text
    Image image;
    Sound sound;
    time_t timestamp;
    bool is_outgoing;
    bool has_media;
} ChatMessage;

typedef struct {
    MessageType type;
    uint32_t size;
} MessageHeader;

// ---------------- Globals ----------------
#define MAX_MESSAGES 100
#define DEFAULT_PORT 8888

static AppState currentState = STATE_MAIN_MENU;
static ChatMessage message_queue[MAX_MESSAGES];
static int message_count = 0;

static int server_socket = -1;
static bool server_running = false;
static pthread_t server_thread = 0;
static pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

static char chat_input_buffer[512] = "";
static char ip_input_buffer[INET_ADDRSTRLEN] = "127.0.0.1";
static char port_input_buffer[8] = "8888";
static bool chat_input_edit_mode = false;
static bool ip_input_edit_mode = false;
static bool port_input_edit_mode = false;

// ---------------- Status Message ----------------
static char status_buffer[256] = "";
static time_t status_time = 0;

void ShowStatusMessage(const char* message) {
    strncpy(status_buffer, message, sizeof(status_buffer)-1);
    status_time = time(NULL);
}

void UpdateStatusMessage(void) {
    if (status_time != 0 && time(NULL) - status_time > 5) {
        status_buffer[0] = '\0';
        status_time = 0;
    }
}

void DrawStatusMessage(void) {
    if (status_buffer[0] != '\0') {
        int sw = GetScreenWidth();
        DrawText(status_buffer, sw/2 - MeasureText(status_buffer, 14)/2,
                 GetScreenHeight() - 20, 14, RED);
    }
}

// ---------------- Chat Management ----------------
void AddChatMessage(const char* sender_ip, const char* content, bool is_outgoing,
                    MessageType type, Image img, Sound snd, bool has_media) {
    pthread_mutex_lock(&message_mutex);
    if (message_count >= MAX_MESSAGES) {
        for (int i = 0; i < MAX_MESSAGES-1; i++) message_queue[i] = message_queue[i+1];
        message_count = MAX_MESSAGES-1;
    }
    ChatMessage* msg = &message_queue[message_count];
    strncpy(msg->sender_ip, sender_ip, sizeof(msg->sender_ip)-1);
    msg->sender_ip[sizeof(msg->sender_ip)-1] = '\0';
    strncpy(msg->content, content ? content : "", sizeof(msg->content)-1);
    msg->content[sizeof(msg->content)-1] = '\0';
    msg->timestamp = time(NULL);
    msg->is_outgoing = is_outgoing;
    msg->type = type;
    msg->has_media = has_media;
    if (type == MSG_IMAGE && has_media) msg->image = img;
    if (type == MSG_AUDIO && has_media) msg->sound = snd;
    message_count++;
    pthread_mutex_unlock(&message_mutex);
}

// ---------------- Networking ----------------
void* ServerThreadFunc(void* arg) {
    int port = *((int*)arg);
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) { ShowStatusMessage("Socket create fail"); return NULL; }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        ShowStatusMessage("Bind fail"); close(server_socket); return NULL;
    }
    if (listen(server_socket, 5) == -1) {
        ShowStatusMessage("Listen fail"); close(server_socket); return NULL;
    }

    server_running = true;
    ShowStatusMessage("Server started");

    while (server_running) {
        int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) break;

        MessageHeader header;
        ssize_t h = recv(client_sock, &header, sizeof(header), MSG_WAITALL);
        if (h != sizeof(header)) { close(client_sock); continue; }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        char* data = malloc(header.size);
        recv(client_sock, data, header.size, MSG_WAITALL);

        if (header.type == MSG_TEXT) {
            data[header.size-1] = '\0';
            AddChatMessage(client_ip, data, false, MSG_TEXT, (Image){0}, (Sound){0}, false);
        } else if (header.type == MSG_IMAGE) {
            char fname[] = "recv.png";
            FILE* f = fopen(fname, "wb"); fwrite(data,1,header.size,f); fclose(f);
            Image img = LoadImage(fname);
            AddChatMessage(client_ip, "[Image]", false, MSG_IMAGE, img, (Sound){0}, true);
        } else if (header.type == MSG_AUDIO) {
            char fname[] = "recv.wav";
            FILE* f = fopen(fname, "wb"); fwrite(data,1,header.size,f); fclose(f);
            Sound snd = LoadSound(fname);
            PlaySound(snd);
            AddChatMessage(client_ip, "[Audio]", false, MSG_AUDIO, (Image){0}, snd, true);
        }
        free(data);
        close(client_sock);
    }
    return NULL;
}

void StartServer(int port) {
    if (server_running) return;
    static int server_port; server_port = port;
    pthread_create(&server_thread, NULL, ServerThreadFunc, &server_port);
}

void StopServer() {
    server_running = false;
    if (server_socket != -1) close(server_socket);
    if (server_thread) pthread_join(server_thread, NULL);
    server_socket = -1; server_thread = 0;
}

// ---------------- Sending ----------------
void SendPacket(const char* ip, int port, MessageType type, const void* data, size_t size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { ShowStatusMessage("Sock fail"); return; }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET; addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ShowStatusMessage("Connect fail"); close(sock); return;
    }
    MessageHeader h = {type,(uint32_t)size};
    send(sock,&h,sizeof(h),0);
    send(sock,data,size,0);
    close(sock);
}

void SendText(const char* ip, int port, const char* msg) {
    SendPacket(ip,port,MSG_TEXT,msg,strlen(msg)+1);
    AddChatMessage(ip,msg,true,MSG_TEXT,(Image){0},(Sound){0},false);
}

void SendImage(const char* ip,int port,const char* path) {
    int sz; FILE* f=fopen(path,"rb"); if(!f){ShowStatusMessage("No img");return;}
    fseek(f,0,SEEK_END); sz=ftell(f); fseek(f,0,SEEK_SET);
    char* buf=malloc(sz); fread(buf,1,sz,f); fclose(f);
    SendPacket(ip,port,MSG_IMAGE,buf,sz); free(buf);
    Image img=LoadImage(path);
    AddChatMessage(ip,"[Image Sent]",true,MSG_IMAGE,img,(Sound){0},true);
}

void SendAudio(const char* ip,int port,const char* path) {
    int sz; FILE* f=fopen(path,"rb"); if(!f){ShowStatusMessage("No aud");return;}
    fseek(f,0,SEEK_END); sz=ftell(f); fseek(f,0,SEEK_SET);
    char* buf=malloc(sz); fread(buf,1,sz,f); fclose(f);
    SendPacket(ip,port,MSG_AUDIO,buf,sz); free(buf);
    Sound snd=LoadSound(path); PlaySound(snd);
    AddChatMessage(ip,"[Audio Sent]",true,MSG_AUDIO,(Image){0},snd,true);
}

// ---------------- UI ----------------
void DrawChatInterface() {
    ClearBackground(RAYWHITE);
    int sw=GetScreenWidth(), sh=GetScreenHeight();

    if (GuiButton((Rectangle){20,20,80,30},"Back")) {
        StopServer(); currentState=STATE_MAIN_MENU; return;
    }

    DrawText("Socket Chat", sw/2-MeasureText("Socket Chat",24)/2, 20, 24, DARKGRAY);

    DrawText("Server:",120,60,16,DARKGRAY);
    DrawText(server_running?"Running":"Stopped",200,60,16,server_running?GREEN:RED);

    if (GuiButton((Rectangle){300,55,80,25},server_running?"Stop":"Start")) {
        if (server_running) { StopServer(); ShowStatusMessage("Stopped"); }
        else StartServer(atoi(port_input_buffer));
    }

    DrawText("Target IP:",120,90,16,DARKGRAY);
    if (GuiTextBox((Rectangle){190,85,140,25},ip_input_buffer,INET_ADDRSTRLEN,ip_input_edit_mode))
        ip_input_edit_mode=!ip_input_edit_mode;

    Rectangle msg_area={20,120,sw-40,sh-220};
    DrawRectangleRec(msg_area,LIGHTGRAY); DrawRectangleLinesEx(msg_area,2,GRAY);

    pthread_mutex_lock(&message_mutex);
    float y=msg_area.y+10;
    for (int i=(message_count>15?message_count-15:0);i<message_count;i++) {
        ChatMessage* m=&message_queue[i];
        struct tm* t=localtime(&m->timestamp); char ts[20];
        strftime(ts,sizeof(ts),"%H:%M:%S",t);
        Color c=m->is_outgoing?BLUE:DARKGREEN;
        DrawText(TextFormat("[%s] %s: %s",ts,m->sender_ip,m->content),msg_area.x+10,y,12,c);
        y+=20;
        if (m->type==MSG_IMAGE && m->has_media) {
            Texture2D tex=LoadTextureFromImage(m->image);
            DrawTextureEx(tex,(Vector2){msg_area.x+20,y},0,0.2f,WHITE);
            y+=m->image.height*0.2f+10;
            UnloadTexture(tex);
        }
        if (m->type==MSG_AUDIO && m->has_media) {
            DrawText("[Audio Clip]",msg_area.x+20,y,12,MAROON);
            y+=20;
        }
    }
    pthread_mutex_unlock(&message_mutex);

    if (GuiTextBox((Rectangle){20,sh-90,sw-200,30},chat_input_buffer,sizeof(chat_input_buffer),chat_input_edit_mode))
        chat_input_edit_mode=!chat_input_edit_mode;
    if (GuiButton((Rectangle){sw-170,sh-90,70,30},"Send"))
        if(strlen(chat_input_buffer)) { SendText(ip_input_buffer,atoi(port_input_buffer),chat_input_buffer); chat_input_buffer[0]=0; }

    if (GuiButton((Rectangle){sw-90,sh-130,70,30},"Img")) SendImage(ip_input_buffer,atoi(port_input_buffer),"test.png");
    if (GuiButton((Rectangle){sw-90,sh-170,70,30},"Aud")) SendAudio(ip_input_buffer,atoi(port_input_buffer),"test.wav");

    DrawStatusMessage();
}

void DrawMainMenu() {
    ClearBackground(RAYWHITE);
    int sw=GetScreenWidth();
    DrawText("StegaNet Enhanced",sw/2-MeasureText("StegaNet Enhanced",40)/2,80,40,DARKGRAY);
    if (GuiButton((Rectangle){sw/2-100,200,200,80},"Socket Chat")) currentState=STATE_CHAT_INTERFACE;
    if (GuiButton((Rectangle){sw/2-50,320,100,40},"Exit")) { StopServer(); exit(0); }
    DrawStatusMessage();
}

// ---------------- Main ----------------
int main(void) {
    InitWindow(1000,650,"StegaNet Enhanced");
    InitAudioDevice(); SetTargetFPS(60);
    while(!WindowShouldClose()) {
        UpdateStatusMessage();
        BeginDrawing();
        if(currentState==STATE_MAIN_MENU) DrawMainMenu();
        else if(currentState==STATE_CHAT_INTERFACE) DrawChatInterface();
        EndDrawing();
    }
    CloseAudioDevice(); CloseWindow(); return 0;
}
