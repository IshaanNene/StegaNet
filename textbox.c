#include "textbox.h"
#include <string.h>
#include <math.h> 

static Rectangle container;
static Rectangle resizer;
static Vector2 lastMouse;
static Color borderColor;
static Font font;
static bool resizing = false;
static bool wordWrap = true;
static const char *text = NULL;

void InitTextBox() {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    static char staticText[] =
        "Text cannot escape this container ...word wrap also works when active so here's "
        "a long text for testing.\n\nLorem ipsum dolor sit amet, consectetur adipiscing elit.";
    text = staticText;

    font = GetFontDefault();
    borderColor = MAROON;
    container = (Rectangle){ 25.0f, 25.0f, screenWidth - 50.0f, screenHeight - 250.0f };
    resizer = (Rectangle){ container.x + container.width - 17, container.y + container.height - 17, 14, 14 };
    lastMouse = (Vector2){ 0.0f, 0.0f };
}

void UpdateTextBox() {
    Vector2 mouse = GetMousePosition();

    const float minWidth = 60;
    const float minHeight = 60;
    const float maxWidth = GetScreenWidth() - 50.0f;
    const float maxHeight = GetScreenHeight() - 160.0f;

    if (IsKeyPressed(KEY_SPACE)) wordWrap = !wordWrap;

    if (CheckCollisionPointRec(mouse, container)) borderColor = Fade(MAROON, 0.4f);
    else if (!resizing) borderColor = MAROON;

    if (resizing) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) resizing = false;

        float width = container.width + (mouse.x - lastMouse.x);
        container.width = fminf(fmaxf(width, minWidth), maxWidth);

        float height = container.height + (mouse.y - lastMouse.y);
        container.height = fminf(fmaxf(height, minHeight), maxHeight);
    } else {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, resizer))
            resizing = true;
    }

    resizer.x = container.x + container.width - 17;
    resizer.y = container.y + container.height - 17;

    lastMouse = mouse;
}

// Draw text using font inside rectangle limits with support for text selection
static void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint)
{
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0;          // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position

    for (int i = 0, k = 0; i < length; i++, k++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n')
        {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;

            if (i + 1 < length) glyphWidth = glyphWidth + spacing;
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE)
        {
            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

            if ((textOffsetX + glyphWidth) > rec.width)
            {
                endLine = (endLine < 1)? i : endLine;
                if (i == endLine) endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                state = !state;
            }
            else if ((i + 1) == length)
            {
                endLine = i;
                state = !state;
            }
            else if (codepoint == '\n') state = !state;

            if (state == DRAW_STATE)
            {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        }
        else
        {
            if (codepoint == '\n')
            {
                if (!wordWrap)
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }
            }
            else
            {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width))
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength)))
                {
                    DrawRectangleRec((Rectangle){ rec.x + textOffsetX - 1, rec.y + textOffsetY, glyphWidth, (float)font.baseSize*scaleFactor }, selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t'))
                {
                    DrawTextCodepoint(font, codepoint, (Vector2){ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, isGlyphSelected? selectTint : tint);
                }
            }

            if (wordWrap && (i == endLine))
            {
                textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
            }
        }

        if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
    }
}

// Draw text using font inside rectangle limits
static void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
    DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, tint, 0, 0, WHITE, WHITE);
}

void DrawTextBox() {
    DrawRectangleLinesEx(container, 3, borderColor);
    DrawTextBoxed(font, text,
        (Rectangle){ container.x + 4, container.y + 4, container.width - 4, container.height - 4 },
        20.0f, 2.0f, wordWrap, GRAY);
    DrawRectangleRec(resizer, borderColor);
}