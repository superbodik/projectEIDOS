#include "OverlayUI.h"
#include <rlgl.h>

void OverlayUI::DrawLoadingScreen(int screenWidth, int screenHeight) {
    DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);
    const char* text = "GENERATING TERRAIN...";
    int w = MeasureText(text, 30);
    DrawText(text, (screenWidth - w) / 2, screenHeight / 2 - 15, 30, WHITE);
}

void OverlayUI::DrawCrosshair(int screenWidth, int screenHeight) {
    DrawText("+", screenWidth / 2 - 5, screenHeight / 2 - 10, 20, Fade(WHITE, 0.8f));
}

void OverlayUI::DrawMainMenuOverlay(int screenHeight) {
    DrawText("EIDOS Engine 0.0.1 (ALPHA_BUILD)", 15, screenHeight - 35, 20, Fade(WHITE, 0.7f));
}

void OverlayUI::DrawWaila(int screenWidth, int screenHeight, int blockID, const std::string& blockName) {
    int w = MeasureText(blockName.c_str(), 20);
    int panelW = w + 80;
    int panelH = 46;
    int px = screenWidth / 2 - panelW / 2;
    int py = 20;
    
    DrawRectangle(px + 4, py + 4, panelW, panelH, Fade(BLACK, 0.3f));
    DrawRectangleGradientV(px, py, panelW, panelH, Fade(DARKGRAY, 0.95f), Fade(BLACK, 0.95f));
    DrawRectangleLinesEx({ (float)px, (float)py, (float)panelW, (float)panelH }, 2, Fade(LIGHTGRAY, 0.6f));

    DrawText(blockName.c_str(), px + 55, py + 13, 20, WHITE);
    
    float aspect = (float)screenWidth / (float)screenHeight;
    rlMatrixMode(RL_PROJECTION); rlPushMatrix(); rlLoadIdentity();
    rlOrtho(-aspect, aspect, -1.0, 1.0, -10.0, 10.0);
    rlMatrixMode(RL_MODELVIEW); rlPushMatrix(); rlLoadIdentity();

    float screenX = ((float)(px + 28) / screenWidth) * 2.0f * aspect - aspect;
    float screenY = 1.0f - ((float)(py + 23) / screenHeight) * 2.0f;

    rlTranslatef(screenX, screenY, 0.0f);
    rlRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef((float)GetTime() * 60.0f, 0.0f, 1.0f, 0.0f);
    rlScalef(0.04f, 0.04f, 0.04f);

    Color bc = GRAY;
    if (blockID == 6) bc = GREEN;
    else if (blockID == 17) bc = DARKGRAY;
    else if (blockID == 100) bc = DARKBROWN;
    else if (blockID == 18) bc = Fade(SKYBLUE, 0.8f);
    else if (blockID == 3) bc = ORANGE;

    DrawCube({ 0,0,0 }, 1, 1, 1, bc);
    DrawCubeWires({ 0,0,0 }, 1.05f, 1.05f, 1.05f, BLACK);

    rlPopMatrix(); rlMatrixMode(RL_PROJECTION); rlPopMatrix(); rlMatrixMode(RL_MODELVIEW);
}