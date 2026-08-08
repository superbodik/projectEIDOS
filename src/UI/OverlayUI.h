#pragma once
#include <raylib.h>
#include <string>

struct SurvivalHud {
    float health = 1.0f;
    float satiety = 1.0f;
    float hydration = 1.0f;
    float bodyTempC = 36.6f;
    float airC = 15.0f;
};

struct HudLayout {
    int gui = 2;
    int slot = 40;
    int pad = 4;
    int hotbarX = 0;
    int hotbarY = 0;
    int hotbarW = 396;
    int iconScale = 2;
    int iconStep = 16;
    int rowW = 158;
};

class OverlayUI {
public:
    static int guiScaleSetting;

    static void DrawLoadingScreen(int screenWidth, int screenHeight, float progress = -1.0f,
        const char* worldName = nullptr, const char* stage = nullptr);
    static void DrawCrosshair(int screenWidth, int screenHeight);
    static void DrawWaila(int screenWidth, int screenHeight, int blockID, const std::string& blockName);
    static void DrawMainMenuOverlay(int screenHeight);
    static HudLayout ComputeLayout(int screenWidth, int screenHeight);
    static void DrawSurvivalHud(int screenWidth, int screenHeight, const SurvivalHud& s);
};
