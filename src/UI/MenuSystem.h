#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <filesystem>

class EidosEngine;

struct SaveSlot {
    std::string name;
    std::string path;
    Texture2D cover;
    bool hasCover;
};

// Вкладки настроек
enum class SettingsTab {
    None,       // Главная страница настроек
    Graphics,   // Графика (FOV, Render Distance)
    Advanced,   // Дополнительно (FPS, etc)
    Controls    // Управление (заглушка)
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
    int activeInputBox = 0;

private:
    EidosEngine* engine;
    std::vector<SaveSlot> foundSaves;

    SettingsTab currentSettingsTab = SettingsTab::None; // Текущая вкладка

    void DrawMainMenu();
    void DrawWorldSelect();
    void DrawCreateWorld();
    void DrawPauseMenu();

    // Новая система настроек
    void DrawSettings();
    void DrawSettingsMain();
    void DrawSettingsGraphics();
    void DrawSettingsAdvanced();

    // Обновленные UI элементы с поддержкой масштаба
    bool DrawButton(const char* text, float yOffsetPct, bool active = true);
    bool DrawSlider(const char* label, float minVal, float maxVal, float* value, float yOffsetPct, bool isInt = true);
    void DrawTextBox(const char* label, char* buffer, int maxLength, float yOffsetPct, int id);

    std::string GetUniqueWorldName(const std::string& baseName);
    void UnloadSaveTextures();
};