#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <filesystem>
#include "../Core/WorldRules.h"

class EidosEngine;

struct SaveSlot {
    std::string name;
    std::string path;
    Texture2D cover;
    bool hasCover;
};

enum class SettingsTab {
    None,
    Graphics,
    Advanced,
    Controls
};

class MenuSystem {
public:
    MenuSystem(EidosEngine* engine);
    ~MenuSystem();

    void Update();
    void Render();

    void RefreshSaveList();

    char inputWorldName[32] = "New World";
    char inputSeed[32] = "1337";
    Difficulty newWorldDifficulty = Difficulty::Survival;
    int activeInputBox = 0;
    float worldSelectScroll = 0.0f;

private:
    EidosEngine* engine;
    std::vector<SaveSlot> foundSaves;

    SettingsTab currentSettingsTab = SettingsTab::None;
    bool settingsFromPause = false;
    float appliedFlash = 0.0f;
    int deleteConfirm = -1;

    static const Color BG_PANEL;
    static const Color BG_DEEP;
    static const Color LINE_SOFT;
    static const Color LINE_HARD;
    static const Color ACCENT;
    static const Color ACCENT_DIM;
    static const Color TEXT_MAIN;
    static const Color TEXT_DIM;
    static const Color DANGER;

    float Scale() const;
    void DrawVignette();
    void DrawPanel(Rectangle r, const char* title);
    void DrawTextBoxed(const char* text, Rectangle box, int fontSize, Color col);
    Rectangle PanelRect(float wUnits, float hUnits) const;

    bool Button(Rectangle r, const char* text, bool enabled = true, Color tint = { 0,0,0,0 });
    bool ButtonRow(Rectangle panel, int index, const char* text, bool enabled = true, Color tint = { 0,0,0,0 });
    bool Toggle(Rectangle panel, int index, const char* label, bool value);
    bool Slider(Rectangle panel, int index, const char* label, float minVal, float maxVal,
        float* value, bool isInt, const char* suffix);
    void TextField(Rectangle panel, int index, const char* label, char* buf, int id);

    void DrawMainMenu();
    void DrawWorldSelect();
    void DrawCreateWorld();
    void DrawPauseMenu();

    void DrawSettings();
    void DrawSettingsMain(Rectangle p);
    void DrawSettingsGraphics(Rectangle p);
    void DrawSettingsAdvanced(Rectangle p);
    void DrawSettingsControls(Rectangle p);

    void ExitSettings();

    std::string GetUniqueWorldName(const std::string& baseName);
    void UnloadSaveTextures();
};
