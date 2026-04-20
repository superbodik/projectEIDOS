#pragma once
#include <raylib.h>
#include <string>

class OverlayUI {
public:
    static void DrawLoadingScreen(int screenWidth, int screenHeight);
    static void DrawCrosshair(int screenWidth, int screenHeight);
    static void DrawWaila(int screenWidth, int screenHeight, int blockID, const std::string& blockName);
    static void DrawMainMenuOverlay(int screenHeight);
};