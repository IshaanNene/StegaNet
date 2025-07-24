
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


extern void DrawMainMenu(void);
extern void DrawImageSteganography(void);
extern void DrawAudioSteganography(void);
extern void ShowStatusMessage(const char* message);
extern void UpdateStatusMessage(void);
extern void DrawStatusMessage(void);


typedef enum {
    STATE_MAIN_MENU,
    STATE_IMAGE_STEGO,
    STATE_AUDIO_STEGO,
    STATE_IMAGE_OPTIONS,
    STATE_AUDIO_OPTIONS,
    STATE_IMAGE_ENCODE,
    STATE_IMAGE_DECODE,
    STATE_AUDIO_ENCODE,
    STATE_AUDIO_DECODE,
    STATE_YOUTUBE_INPUT,
    STATE_CHAT_INTERFACE
} AppState;

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 512
#define DEFAULT_PORT 8888


typedef struct {
    char sender_ip[INET_ADDRSTRLEN];
    char content[MAX_MESSAGE_LENGTH];
    time_t timestamp;
    bool is_outgoing;
} ChatMessage;


static int server_socket = -1;
static bool server_running = false;
static pthread_t server_thread = 0;
static pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;


static ChatMessage message_queue[MAX_MESSAGES];
static int message_count = 0;
static char chat_input_buffer[MAX_MESSAGE_LENGTH] = "";
static char ip_input_buffer[INET_ADDRSTRLEN] = "127.0.0.1";
static char port_input_buffer[8] = "8888";
static bool chat_input_edit_mode = false;
static bool ip_input_edit_mode = false;
static bool port_input_edit_mode = false;


extern AppState currentState;


void AddChatMessage(const char* sender_ip, const char* content, bool is_outgoing) {
    pthread_mutex_lock(&message_mutex);
    
    if (message_count >= MAX_MESSAGES) {
        
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            message_queue[i] = message_queue[i + 1];
        }
        message_count = MAX_MESSAGES - 1;
    }
    
    ChatMessage* msg = &message_queue[message_count];
    strncpy(msg->sender_ip, sender_ip, sizeof(msg->sender_ip) - 1);
    msg->sender_ip[sizeof(msg->sender_ip) - 1] = '\0';
    strncpy(msg->content, content, sizeof(msg->content) - 1);
    msg->content[sizeof(msg->content) - 1] = '\0';
    msg->timestamp = time(NULL);
    msg->is_outgoing = is_outgoing;
    
    message_count++;
    
    pthread_mutex_unlock(&message_mutex);
}


void* ServerThreadFunc(void* arg) {
    int port = *((int*)arg);
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        ShowStatusMessage("Failed to create server socket");
        return NULL;
    }
    
    
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        ShowStatusMessage("Failed to bind server socket");
        close(server_socket);
        server_socket = -1;
        return NULL;
    }
    
    if (listen(server_socket, 5) == -1) {
        ShowStatusMessage("Failed to listen on server socket");
        close(server_socket);
        server_socket = -1;
        return NULL;
    }
    
    server_running = true;
    ShowStatusMessage("Server started, listening for connections...");
    
    while (server_running) {
        int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == -1) {
            if (server_running) {  
                ShowStatusMessage("Accept failed");
            }
            break;
        }
        
        char buffer[MAX_MESSAGE_LENGTH];
        ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            AddChatMessage(client_ip, buffer, false);
        }
        close(client_sock);
    }
    
    return NULL;
}

void StartServer(int port) {
    if (server_running) {
        return;
    }
    
    static int server_port; 
    server_port = port;
    
    if (pthread_create(&server_thread, NULL, ServerThreadFunc, &server_port) != 0) {
        ShowStatusMessage("Failed to create server thread");
        return;
    }
}

void StopServer() {
    if (!server_running) {
        return;
    }
    
    server_running = false;
    
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    
    if (server_thread != 0) {
        pthread_join(server_thread, NULL);
        server_thread = 0;
    }
}

void SendMessage(const char* ip, int port, const char* message) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        ShowStatusMessage("Failed to create client socket");
        return;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        ShowStatusMessage("Invalid IP address");
        close(sock);
        return;
    }
    
    
    struct timeval timeout;
    timeout.tv_sec = 5;  
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to connect to %s:%d", ip, port);
        ShowStatusMessage(error_msg);
        close(sock);
        return;
    }
    
    ssize_t bytes_sent = send(sock, message, strlen(message), 0);
    if (bytes_sent > 0) {
        AddChatMessage(ip, message, true);
        ShowStatusMessage("Message sent successfully!");
    } else {
        ShowStatusMessage("Failed to send message");
    }
    
    close(sock);
}


void DrawChatInterface() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    
    if (GuiButton((Rectangle){ 20, 20, 80, 30 }, "Back")) {
        StopServer();
        currentState = STATE_MAIN_MENU;
        return;
    }

    
    const char* title = "Socket Chat Interface";
    int titleWidth = MeasureText(title, 24);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 20, 24, DARKGRAY);

    
    DrawText("Server Status:", 120, 60, 16, DARKGRAY);
    const char* server_status = server_running ? "Running" : "Stopped";
    Color status_color = server_running ? GREEN : RED;
    DrawText(server_status, 220, 60, 16, status_color);

    if (GuiButton((Rectangle){ 300, 55, 80, 25 }, server_running ? "Stop" : "Start")) {
        if (server_running) {
            StopServer();
            ShowStatusMessage("Server stopped");
        } else {
            int port = atoi(port_input_buffer);
            if (port > 0 && port < 65536) {
                StartServer(port);
            } else {
                ShowStatusMessage("Invalid port number!");
            }
        }
    }

    
    DrawText("Port:", 400, 60, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ 440, 55, 80, 25 }, port_input_buffer, 8, port_input_edit_mode)) {
        port_input_edit_mode = !port_input_edit_mode;
    }

    
    DrawText("Target IP:", 120, 90, 16, DARKGRAY);
    if (GuiTextBox((Rectangle){ 190, 85, 140, 25 }, ip_input_buffer, INET_ADDRSTRLEN, ip_input_edit_mode)) {
        ip_input_edit_mode = !ip_input_edit_mode;
    }

    DrawText("Target Port:", 340, 90, 16, DARKGRAY);
    char target_port_buffer[8];
    strncpy(target_port_buffer, port_input_buffer, sizeof(target_port_buffer));
    if (GuiTextBox((Rectangle){ 420, 85, 80, 25 }, target_port_buffer, 8, false)) {
        
    }

    
    Rectangle message_area = { 20, 120, screenWidth - 40, screenHeight - 220 };
    DrawRectangleRec(message_area, LIGHTGRAY);
    DrawRectangleLinesEx(message_area, 2, GRAY);

    
    pthread_mutex_lock(&message_mutex);
    float y_offset = message_area.y + 10;
    int visible_messages = (int)((message_area.height - 20) / 22);
    int start_message = (message_count > visible_messages) ? message_count - visible_messages : 0;

    for (int i = start_message; i < message_count; i++) {
        ChatMessage* msg = &message_queue[i];
        
        
        struct tm* time_info = localtime(&msg->timestamp);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);
        
        
        Color msg_color = msg->is_outgoing ? BLUE : DARKGREEN;
        
        
        char display_msg[600];
        const char* direction = msg->is_outgoing ? "→" : "←";
        snprintf(display_msg, sizeof(display_msg), "[%s] %s %s: %.60s", 
                time_str, direction, msg->sender_ip, msg->content);
        
        
        if (strlen(msg->content) > 60) {
            strcat(display_msg, "...");
        }
        
        DrawText(display_msg, message_area.x + 10, y_offset, 12, msg_color);
        y_offset += 22;
        
        if (y_offset > message_area.y + message_area.height - 25) {
            break;
        }
    }
    pthread_mutex_unlock(&message_mutex);

    
    Rectangle input_area = { 20, screenHeight - 90, screenWidth - 140, 30 };
    DrawText("Message:", 20, screenHeight - 110, 16, DARKGRAY);
    
    if (GuiTextBox(input_area, chat_input_buffer, MAX_MESSAGE_LENGTH, chat_input_edit_mode)) {
        chat_input_edit_mode = !chat_input_edit_mode;
    }

    
    if (GuiButton((Rectangle){ screenWidth - 110, screenHeight - 90, 80, 30 }, "Send")) {
        if (strlen(chat_input_buffer) > 0 && strlen(ip_input_buffer) > 0) {
            int port = atoi(port_input_buffer);
            if (port > 0 && port < 65536) {
                SendMessage(ip_input_buffer, port, chat_input_buffer);
                chat_input_buffer[0] = '\0'; 
                chat_input_edit_mode = false;
            } else {
                ShowStatusMessage("Invalid port number!");
            }
        } else {
            ShowStatusMessage("Please enter a message and target IP!");
        }
    }

    
    const char* instructions = "Start server to receive messages. Enter target IP and click Send to send messages.";
    int instrWidth = MeasureText(instructions, 12);
    DrawText(instructions, screenWidth / 2 - instrWidth / 2, screenHeight - 50, 12, GRAY);

    
    char connection_info[200];
    snprintf(connection_info, sizeof(connection_info), "Your IP: Use 'ifconfig' in terminal to find your local IP address");
    DrawText(connection_info, 20, screenHeight - 30, 12, DARKGRAY);

    DrawStatusMessage();
}


void DrawEnhancedMainMenu() {
    ClearBackground(RAYWHITE);
    int screenWidth = GetScreenWidth();
    
    
    const char* title = "StegaNet Enhanced";
    int titleWidth = MeasureText(title, 40);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 80, 40, DARKGRAY);
    
    
    const char* subtitle = "Steganography + Socket Communication";
    int subtitleWidth = MeasureText(subtitle, 20);
    DrawText(subtitle, screenWidth / 2 - subtitleWidth / 2, 130, 20, GRAY);

    
    int buttonWidth = 200;
    int buttonHeight = 80;
    int spacing = 40;
    
    
    if (GuiButton((Rectangle){ screenWidth / 2 - buttonWidth - spacing / 2, 200, buttonWidth, buttonHeight }, "Image\nSteganography")) {
        currentState = STATE_IMAGE_STEGO;
    }
    if (GuiButton((Rectangle){ screenWidth / 2 + spacing / 2, 200, buttonWidth, buttonHeight }, "Audio\nSteganography")) {
        currentState = STATE_AUDIO_STEGO;
    }
    
    
    if (GuiButton((Rectangle){ screenWidth / 2 - buttonWidth / 2, 320, buttonWidth, buttonHeight }, "Socket Chat\nInterface")) {
        currentState = STATE_CHAT_INTERFACE;
    }
    
    
    if (GuiButton((Rectangle){ screenWidth / 2 - 50, 450, 100, 40 }, "Exit")) {
        StopServer();
        pthread_mutex_destroy(&message_mutex);
        CloseWindow();
        exit(0);
    }

    DrawStatusMessage();
}


void CleanupSocketChat() {
    StopServer();
    pthread_mutex_destroy(&message_mutex);
}


int main(void) {
    InitWindow(1000, 650, "StegaNet Enhanced - Steganography + Socket Chat");
    InitAudioDevice();
    SetTargetFPS(60);

    
    currentState = STATE_MAIN_MENU;

    while (!WindowShouldClose()) {
        UpdateStatusMessage();

        BeginDrawing();

        switch (currentState) {
            case STATE_MAIN_MENU:
                DrawEnhancedMainMenu();
                break;
            case STATE_CHAT_INTERFACE:
                DrawChatInterface();
                break;
            case STATE_IMAGE_STEGO:
                DrawImageSteganography();
                break;
            case STATE_AUDIO_STEGO:
                DrawAudioSteganography();
                break;
            
            
            default:
                
                currentState = STATE_MAIN_MENU;
                break;
        }

        EndDrawing();
    }

    
    CleanupSocketChat();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}