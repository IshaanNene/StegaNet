#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include <time.h>

#include "utils.h"     // ShowStatusMessage/UpdateStatusMessage/DrawStatusMessage (already in your repo)
#include "state.h"     // AppState (we’ll add STATE_CHAT below)

/* ---------------------- Protocol ---------------------- */
typedef enum { MSG_TEXT = 1, MSG_IMAGE = 2, MSG_AUDIO = 3 } MsgType;

typedef struct __attribute__((packed)) {
    uint32_t type;   // MsgType
    uint32_t size;   // payload size in bytes
} MsgHeader;

/* ---------------------- Message model ---------------------- */
typedef struct {
    bool      outgoing;
    char      from[INET_ADDRSTRLEN];
    time_t    ts;
    MsgType   type;

    // text
    char      text[512];

    // image
    Texture2D tex;
    bool      hasTex;

    // audio
    Sound     snd;
    bool      hasSnd;
} ChatMsg;

#define MAX_MSG 150

/* ---------------------- Chat state ---------------------- */
static ChatMsg msgs[MAX_MSG];
static int msgCount = 0;
static pthread_mutex_t msgMx = PTHREAD_MUTEX_INITIALIZER;

/* inputs */
static char myPortStr[8] = "8888";
static char peerIpStr[INET_ADDRSTRLEN] = "127.0.0.1";
static char peerPortStr[8] = "8888";
static char textBuf[512] = "";

static bool myPortEdit=false, peerIpEdit=false, peerPortEdit=false, textEdit=false;

/* server */
static volatile bool serverRunning = false;
static int serverSock = -1;
static pthread_t serverThread;

/* layout helpers */
static Rectangle R(double x,double y,double w,double h){ return (Rectangle){(float)x,(float)y,(float)w,(float)h}; }

/* ---------------------- Utils ---------------------- */
static void AddMsg(ChatMsg m){
    pthread_mutex_lock(&msgMx);
    if (msgCount >= MAX_MSG) {
        // unload old media if any
        if (msgs[0].hasTex) UnloadTexture(msgs[0].tex);
        if (msgs[0].hasSnd) UnloadSound(msgs[0].snd);
        memmove(&msgs[0], &msgs[1], sizeof(ChatMsg)*(MAX_MSG-1));
        msgCount = MAX_MSG-1;
    }
    msgs[msgCount++] = m;
    pthread_mutex_unlock(&msgMx);
}

static int send_all(int sock, const void* buf, int len){
    const char* p=(const char*)buf; int left=len;
    while(left>0){
        int n=send(sock,p,left,0);
        if(n<=0) return n;
        p+=n; left-=n;
    }
    return len;
}
static int recv_all(int sock, void* buf, int len){
    char* p=(char*)buf; int left=len;
    while(left>0){
        int n=recv(sock,p,left,0);
        if(n<=0) return n;
        p+=n; left-=n;
    }
    return len;
}

static const char* LocalIP(void){
    // UDP trick to discover outbound interface IP
    static char ip[INET_ADDRSTRLEN]="0.0.0.0";
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s<0) return ip;
    struct sockaddr_in tmp={0};
    tmp.sin_family=AF_INET; tmp.sin_port=htons(53);
    inet_pton(AF_INET,"8.8.8.8",&tmp.sin_addr);
    connect(s,(struct sockaddr*)&tmp,sizeof(tmp));
    struct sockaddr_in name={0}; socklen_t namelen=sizeof(name);
    getsockname(s,(struct sockaddr*)&name,&namelen);
    inet_ntop(AF_INET,&name.sin_addr,ip,sizeof(ip));
    close(s);
    return ip;
}

/* ---------------------- Sending ---------------------- */
static void SendPacket(const char* ip, int port, MsgType type, const void* data, uint32_t size){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){ ShowStatusMessage("socket() failed"); return; }
    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,ip,&addr.sin_addr)<=0){ ShowStatusMessage("Bad IP"); close(sock); return; }
    if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))<0){ ShowStatusMessage("connect() failed"); close(sock); return; }

    MsgHeader h = { (uint32_t)type, size };
    if(send_all(sock,&h,sizeof(h))<=0 || (size>0 && send_all(sock,data,size)<=0)){
        ShowStatusMessage("send failed");
        close(sock);
        return;
    }
    close(sock);
}

static void SendText(const char* ip, int port, const char* s){
    SendPacket(ip,port,MSG_TEXT,s,(uint32_t)(strlen(s)+1));

    ChatMsg m = {0};
    m.outgoing=true; strncpy(m.from,LocalIP(),sizeof(m.from)-1);
    m.ts=time(NULL); m.type=MSG_TEXT;
    strncpy(m.text,s,sizeof(m.text)-1);
    AddMsg(m);
}

static void SendImageFile(const char* ip,int port,const char* path){
    FILE* f=fopen(path,"rb"); if(!f){ ShowStatusMessage("Image open fail"); return; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    char* buf=(char*)malloc(sz); fread(buf,1,sz,f); fclose(f);

    SendPacket(ip,port,MSG_IMAGE,buf,(uint32_t)sz);
    free(buf);

    Image img = LoadImage(path);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    ChatMsg m={0}; m.outgoing=true; strncpy(m.from,LocalIP(),sizeof(m.from)-1);
    m.ts=time(NULL); m.type=MSG_IMAGE; m.hasTex=true; m.tex=tex;
    strcpy(m.text,"[image]");
    AddMsg(m);
}

static void SendAudioFile(const char* ip,int port,const char* path){
    FILE* f=fopen(path,"rb"); if(!f){ ShowStatusMessage("Audio open fail"); return; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    char* buf=(char*)malloc(sz); fread(buf,1,sz,f); fclose(f);

    SendPacket(ip,port,MSG_AUDIO,buf,(uint32_t)sz);
    free(buf);

    Sound s = LoadSound(path);
    ChatMsg m={0}; m.outgoing=true; strncpy(m.from,LocalIP(),sizeof(m.from)-1);
    m.ts=time(NULL); m.type=MSG_AUDIO; m.hasSnd=true; m.snd=s;
    strcpy(m.text,"[audio]");
    AddMsg(m);
}

/* ---------------------- Random Generators ---------------------- */
static Texture2D MakeRandomImageTexture(int w,int h){
    Image img = GenImageColor(w,h,BLACK);
    // random pixels
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = LoadImageColors(img);
    for(int i=0;i<w*h;i++){
        px[i] = (Color){ (unsigned char)GetRandomValue(0,255),
                         (unsigned char)GetRandomValue(0,255),
                         (unsigned char)GetRandomValue(0,255), 255 };
    }
    UnloadImageColors(px);
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    return t;
}
static void SendRandomImage(const char* ip,int port){
    // Save to a temp png so receiver can load it as an image from bytes
    int w=256,h=256;
    Image img = GenImageColor(w,h,BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = LoadImageColors(img);
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            int idx=y*w+x;
            px[idx]=(Color){ (unsigned char)(x*255/w),
                             (unsigned char)GetRandomValue(0,255),
                             (unsigned char)(y*255/h), 255 };
        }
    }
    ImageDrawPixelV(&img,(Vector2){(float)GetRandomValue(0,w-1),(float)GetRandomValue(0,h-1)},WHITE);
    ExportImage(img,"__rand_img.png");

    SendImageFile(ip,port,"__rand_img.png");
    // local preview already handled by SendImageFile
    RemoveFile("__rand_img.png");
    UnloadImage(img);
}

static Sound MakeRandomTone(float sec){
    int sampleRate=44100;
    int sampleCount=(int)(sec*sampleRate);
    short* data = (short*)MemAlloc(sampleCount*sizeof(short));
    float freq = (float)GetRandomValue(220,880);
    for(int i=0;i<sampleCount;i++){
        float t = (float)i/sampleRate;
        float s = sinf(2.0f*PI*freq*t);
        data[i] = (short)(s*30000);
        // add a touch of noise
        data[i] = (short)CLAMP((int)data[i] + GetRandomValue(-800,800), -32768, 32767);
    }
    Wave w = {
        .frameCount = sampleCount,
        .sampleRate = sampleRate,
        .sampleSize = 16,
        .channels   = 1,
        .data       = data
    };
    ExportWave(w,"__rand.wav");
    Sound snd = LoadSoundFromWave(w);
    UnloadWave(w);
    MemFree(data);
    PlaySound(snd); // local feedback
    return snd;
}
static void SendRandomAudio(const char* ip,int port){
    Sound s = MakeRandomTone(1.0f);
    // re-open saved file to send bytes
    SendAudioFile(ip,port,"__rand.wav");
    RemoveFile("__rand.wav");
}

/* ---------------------- Server thread ---------------------- */
static void* ServerThread(void* arg){
    int port = *(int*)arg;

    serverSock = socket(AF_INET,SOCK_STREAM,0);
    if(serverSock<0){ ShowStatusMessage("server socket() failed"); return NULL; }

    int opt=1; setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(port);

    if(bind(serverSock,(struct sockaddr*)&addr,sizeof(addr))<0){
        ShowStatusMessage("bind() failed");
        close(serverSock); serverSock=-1; return NULL;
    }
    if(listen(serverSock,8)<0){
        ShowStatusMessage("listen() failed");
        close(serverSock); serverSock=-1; return NULL;
    }

    serverRunning=true;
    ShowStatusMessage("Server running");

    while(serverRunning){
        struct sockaddr_in cli; socklen_t clen=sizeof(cli);
        int cs = accept(serverSock,(struct sockaddr*)&cli,&clen);
        if(cs<0){ if(serverRunning) ShowStatusMessage("accept() failed"); break; }

        MsgHeader h;
        if(recv_all(cs,&h,sizeof(h))!=sizeof(h)){ close(cs); continue; }

        char peer[INET_ADDRSTRLEN]="";
        inet_ntop(AF_INET,&cli.sin_addr,peer,sizeof(peer));

        char* payload = NULL;
        if(h.size>0){ payload = (char*)malloc(h.size); if(recv_all(cs,payload,h.size)!= (int)h.size){ free(payload); close(cs); continue; } }

        if(h.type==MSG_TEXT){
            ChatMsg m={0}; m.outgoing=false; strncpy(m.from,peer,sizeof(m.from)-1);
            m.ts=time(NULL); m.type=MSG_TEXT;
            if(payload){ strncpy(m.text,payload,sizeof(m.text)-1); }
            AddMsg(m);
        } else if(h.type==MSG_IMAGE){
            // write to temp file then load image → texture
            const char* tmp="__rx.png";
            FILE* f=fopen(tmp,"wb");
            if(f){ fwrite(payload,1,h.size,f); fclose(f); }
            Image img = LoadImage(tmp);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            ChatMsg m={0}; m.outgoing=false; strncpy(m.from,peer,sizeof(m.from)-1);
            m.ts=time(NULL); m.type=MSG_IMAGE; m.hasTex=true; m.tex=tex; strcpy(m.text,"[image]");
            AddMsg(m);
            RemoveFile(tmp);
        } else if(h.type==MSG_AUDIO){
            const char* tmp="__rx.wav";
            FILE* f=fopen(tmp,"wb");
            if(f){ fwrite(payload,1,h.size,f); fclose(f); }
            Sound s = LoadSound(tmp);
            ChatMsg m={0}; m.outgoing=false; strncpy(m.from,peer,sizeof(m.from)-1);
            m.ts=time(NULL); m.type=MSG_AUDIO; m.hasSnd=true; m.snd=s; strcpy(m.text,"[audio]");
            AddMsg(m);
            // optional auto-play on receive:
            // PlaySound(s);
            // keep tmp for duration of program? we loaded into memory, so file can be removed:
            RemoveFile(tmp);
        }
        if(payload) free(payload);
        close(cs);
    }
    if(serverSock!=-1){ close(serverSock); serverSock=-1; }
    serverRunning=false;
    return NULL;
}

/* ---------------------- Public API ---------------------- */
void Chat_Init(void){
    // seed PRNG for random generators
    SetRandomSeed((unsigned int)time(NULL));
}
void Chat_Shutdown(void){
    // stop server
    serverRunning=false;
    if(serverSock!=-1){ close(serverSock); serverSock=-1; }
    if(serverThread) pthread_join(serverThread,NULL);

    // free media
    for(int i=0;i<msgCount;i++){
        if(msgs[i].hasTex) UnloadTexture(msgs[i].tex);
        if(msgs[i].hasSnd) UnloadSound(msgs[i].snd);
    }
    msgCount=0;
}
void Chat_Update(void){
    UpdateStatusMessage();

    // drag & drop sending (Raylib-only)
    if(IsFileDropped()){
        FilePathList files = LoadDroppedFiles();
        if(files.count>0){
            const char* p = files.paths[0];
            int port = atoi(peerPortStr);
            if (TextIsEqual(GetFileExtension(p), ".png") || TextIsEqual(GetFileExtension(p), ".jpg") || TextIsEqual(GetFileExtension(p), ".jpeg")) {
                SendImageFile(peerIpStr,port,p);
            } else if (TextIsEqual(GetFileExtension(p), ".wav") || TextIsEqual(GetFileExtension(p), ".ogg") || TextIsEqual(GetFileExtension(p), ".mp3")) {
                // Raylib loads wav/ogg; for mp3 sending is fine, but playing relies on your build options
                SendAudioFile(peerIpStr,port,p);
            } else {
                ShowStatusMessage("Drop an image (.png/.jpg) or audio (.wav/.ogg)");
            }
        }
        UnloadDroppedFiles(files);
    }
}
void Chat_Draw(void){
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    ClearBackground(RAYWHITE);
    DrawText("Socket Chat", sw/2 - MeasureText("Socket Chat", 28)/2, 12, 28, DARKGRAY);

    // Left: server controls (my listen)
    DrawText("My Port:", 20, 60, 18, DARKGRAY);
    if (GuiTextBox(R(95,55,100,28), myPortStr, sizeof(myPortStr), myPortEdit)) myPortEdit = !myPortEdit;

    if (!serverRunning) {
        if (GuiButton(R(210,55,80,28), "Start")) {
            int port = atoi(myPortStr);
            if (port <= 0 || port > 65535) ShowStatusMessage("Invalid port");
            else {
                static int argPort;
                argPort = port;
                pthread_create(&serverThread,NULL,ServerThread,&argPort);
            }
        }
    } else {
        DrawText("Running", 210, 60, 18, GREEN);
        if (GuiButton(R(300,55,80,28), "Stop")) {
            serverRunning=false;
            if (serverSock!=-1){ close(serverSock); serverSock=-1; }
            if (serverThread) { pthread_join(serverThread,NULL); serverThread=0; }
            ShowStatusMessage("Server stopped");
        }
    }

    DrawText(TextFormat("My IP: %s", LocalIP()), 20, 90, 16, GRAY);

    // Right: peer (destination)
    DrawText("Peer IP:", sw-420, 60, 18, DARKGRAY);
    if (GuiTextBox(R(sw-340,55,160,28), peerIpStr, sizeof(peerIpStr), peerIpEdit)) peerIpEdit=!peerIpEdit;

    DrawText("Peer Port:", sw-170, 60, 18, DARKGRAY);
    if (GuiTextBox(R(sw-90,55,70,28), peerPortStr, sizeof(peerPortStr), peerPortEdit)) peerPortEdit=!peerPortEdit;

    // Messages area
    Rectangle list = R(20, 120, sw-40, sh-220);
    DrawRectangleRec(list, LIGHTGRAY);
    DrawRectangleLinesEx(list, 2, GRAY);
    int y = (int)list.y + 8;

    pthread_mutex_lock(&msgMx);
    int start = msgCount - 100; if (start < 0) start = 0;
    for (int i=start; i<msgCount; i++){
        ChatMsg* m = &msgs[i];
        struct tm* ti = localtime(&m->ts);
        char tbuf[16]; strftime(tbuf,sizeof(tbuf),"%H:%M:%S",ti);

        Color c = m->outgoing ? BLUE : DARKGREEN;
        DrawText(TextFormat("(%s) %s: %s", tbuf, m->from, m->text), list.x+8, y, 14, c);
        y += 18;

        if (m->type==MSG_IMAGE && m->hasTex){
            float scale = 0.25f;
            int th = (int)(m->tex.height*scale);
            DrawTextureEx(m->tex, (Vector2){ list.x+16, (float)y }, 0, scale, WHITE);
            y += th + 8;
        }
        if (m->type==MSG_AUDIO && m->hasSnd){
            if (GuiButton(R(list.x+16, y, 80, 24), "Play")) PlaySound(m->snd);
            y += 32;
        }

        if (y > list.y + list.height - 40) break;
    }
    pthread_mutex_unlock(&msgMx);

    // Input + action bar
    if (GuiTextBox(R(20, sh-86, sw-320, 32), textBuf, sizeof(textBuf), textEdit)) textEdit=!textEdit;
    if (GuiButton(R(sw-290, sh-86, 80, 32), "Send")) {
        if (textBuf[0]) {
            SendText(peerIpStr, atoi(peerPortStr), textBuf);
            textBuf[0]=0;
        }
    }
    if (GuiButton(R(sw-200, sh-86, 80, 32), "Img")) {
        // Send last screenshot if exists, otherwise random image
        SendRandomImage(peerIpStr, atoi(peerPortStr));
    }
    if (GuiButton(R(sw-110, sh-86, 80, 32), "Aud")) {
        SendRandomAudio(peerIpStr, atoi(peerPortStr));
    }

    DrawText("Tip: Drag & drop an image (.png/.jpg) or audio (.wav/.ogg) onto the window to send.",
             20, sh-44, 14, GRAY);

    DrawStatusMessage();
}
