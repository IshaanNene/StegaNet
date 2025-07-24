#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib copy/paste example");
    SetTargetFPS(60);

    const char* textToCopy = "This is the text to copy!";
    SetClipboardText(textToCopy); 

    char* clipboardText = GetClipboardText(); 

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Check your clipboard (CTRL+V)", 10, 10, 20, DARKGRAY);
        
        if (clipboardText) {
            DrawText(clipboardText, 10, 50, 20, DARKGRAY); 
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}