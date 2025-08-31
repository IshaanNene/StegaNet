#include "globals.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void DownloadFromYouTube(const char* url) {
    char command[1024];
    snprintf(command, sizeof(command), "python3 yt_downloader.py \"%s\"", url);
    int result = system(command);
    if (result == 0) {
        ShowStatusMessage("Audio downloaded successfully!");
    } else {
        ShowStatusMessage("Failed to download audio. Check yt-dlp or URL.");
    }
}
