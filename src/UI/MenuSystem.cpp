#include "MenuSystem.h"
#include "../Core/EidosEngine.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

MenuSystem::MenuSystem(EidosEngine* ptr) : engine(ptr) {
    RefreshSaveList();
}

MenuSystem::~MenuSystem() {
    UnloadSaveTextures();
}

void MenuSystem::UnloadSaveTextures() {
    for (auto& slot : foundSaves) {
        if (slot.hasCover) UnloadTexture(slot.cover);
    }
    foundSaves.clear();
}

void MenuSystem::RefreshSaveList() {
    UnloadSaveTextures();
    std::string path = "saves";
    if (!fs::exists(path)) fs::create_directories(path);

    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            std::string fullPath = entry.path().string();
            SaveSlot slot; slot.name = name; slot.path = fullPath; slot.hasCover = false;
            if (fs::exists(fullPath + "/level.dat")) {
                std::string imgPath = fullPath + "/cover.png";
                if (fs::exists(imgPath)) {
                    slot.cover = LoadTexture(imgPath.c_str());
                    if (slot.cover.id > 0) slot.hasCover = true;
                }
                foundSaves.push_back(slot);
            }
        }
    }
}

std::string MenuSystem::GetUniqueWorldName(const std::string& baseName) {
    std::string finalName = baseName;
    int counter = 1;
    while (fs::exists("saves/" + finalName)) {
        finalName = baseName + " " + std::to_string(counter);
        counter++;
    }
    return finalName;
}

void MenuSystem::Update() {
    if (engine->currentState == GameState::CreateWorld) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125)) {
                char* target = (activeInputBox == 0) ? inputWorldName : inputSeed;
                int len = (int)strlen(target);
                if (len < 31) { target[len] = (char)key; target[len + 1] = '\0'; }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            char* target = (activeInputBox == 0) ? inputWorldName : inputSeed;
            int len = (int)strlen(target);
            if (len > 0) target[len - 1] = '\0';
        }
        if (IsKeyPressed(KEY_TAB)) activeInputBox = (activeInputBox == 0) ? 1 : 0;
    }
}

void MenuSystem::Render() {
    switch (engine->currentState) {
    case GameState::MainMenu: DrawMainMenu(); break;
    case GameState::Settings: DrawSettings(); break;
    case GameState::WorldSelect: DrawWorldSelect(); break;
    case GameState::CreateWorld: DrawCreateWorld(); break;
    case GameState::Paused: DrawPauseMenu(); break;
    default: break;
    }
}


bool MenuSystem::DrawButton(const char* text, float yOffsetPct, bool active) {
    if (!active) return false;

    float scale = engine->GetUIScale();
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float btnWidth = 350.0f * scale;
    float btnHeight = 60.0f * scale;
    float fontSize = 30.0f * scale;
    float borderThick = 3.0f * scale;

    float x = (sw - btnWidth) / 2.0f;
    float y = (sh / 2.0f) + (sh * yOffsetPct);

    Rectangle rect = { x, y, btnWidth, btnHeight };
    bool isHover = CheckCollisionPointRec(GetMousePosition(), rect);

    DrawRectangleRec(rect, isHover ? Fade(WHITE, 0.9f) : Fade(WHITE, 0.7f));
    DrawRectangleLinesEx(rect, borderThick, isHover ? BLUE : GRAY);

    int textW = MeasureText(text, (int)fontSize);
    DrawText(text, (int)(x + (btnWidth - textW) / 2), (int)(y + (btnHeight - fontSize) / 2), (int)fontSize, isHover ? BLUE : DARKGRAY);

    return isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool MenuSystem::DrawSlider(const char* label, float minVal, float maxVal, float* value, float yOffsetPct, bool isInt) {
    float scale = engine->GetUIScale();
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float w = 350.0f * scale;
    float h = 25.0f * scale;
    float fontSize = 24.0f * scale;

    float x = (sw - w) / 2.0f;
    float y = (sh / 2.0f) + (sh * yOffsetPct);

    if (isInt) DrawText(TextFormat("%s: %d", label, (int)*value), (int)x, (int)(y - fontSize - 5 * scale), (int)fontSize, WHITE);
    else DrawText(TextFormat("%s: %.1f", label, *value), (int)x, (int)(y - fontSize - 5 * scale), (int)fontSize, WHITE);

    Rectangle bar = { x, y, w, h };
    DrawRectangleRec(bar, DARKGRAY);
    DrawRectangleLinesEx(bar, 2 * scale, GRAY);

    float normalized = (*value - minVal) / (maxVal - minVal);
    float knobW = 25.0f * scale;
    float knobH = 35.0f * scale;
    Rectangle knob = { x + normalized * (w - knobW), y - (knobH - h) / 2, knobW, knobH };

    bool hover = CheckCollisionPointRec(GetMousePosition(), bar) || CheckCollisionPointRec(GetMousePosition(), knob);

    if (hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float mouseX = GetMousePosition().x;
        float newValNorm = (mouseX - x) / w;
        if (newValNorm < 0) newValNorm = 0;
        if (newValNorm > 1) newValNorm = 1;

        *value = minVal + (newValNorm * (maxVal - minVal));
        if (isInt) *value = (float)((int)*value);
        return true;
    }

    DrawRectangleRec(knob, hover ? BLUE : LIGHTGRAY);
    DrawRectangleLinesEx(knob, 2 * scale, WHITE);

    return false;
}

void MenuSystem::DrawTextBox(const char* label, char* buffer, int, float yOffsetPct, int id) {
    float scale = engine->GetUIScale();
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float w = 350.0f * scale;
    float h = 50.0f * scale;
    float fontSize = 24.0f * scale;

    float x = (sw - w) / 2.0f;
    float y = (sh / 2.0f) + (sh * yOffsetPct);

    DrawText(label, (int)x, (int)(y - fontSize - 5 * scale), (int)fontSize, WHITE);
    Rectangle rect = { x, y, w, h };
    bool isActive = (activeInputBox == id);

    DrawRectangleRec(rect, isActive ? Fade(WHITE, 0.9f) : Fade(LIGHTGRAY, 0.5f));
    DrawRectangleLinesEx(rect, 2 * scale, isActive ? GREEN : GRAY);
    DrawText(buffer, (int)(x + 5 * scale), (int)(y + 12 * scale), (int)fontSize, BLACK);

    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activeInputBox = id;
    }
}

void MenuSystem::DrawMainMenu() {
    // Получаем АКТУАЛЬНЫЕ размеры окна
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = engine->GetUIScale();

    // Рисуем фон на ВЕСЬ экран
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.4f));

    const char* title = "E I D O S";
    int titleSize = (int)(90 * scale);
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (sw - titleW) / 2, (int)(sh * 0.2f), titleSize, WHITE);
    DrawText("ENGINE V0.5", (sw - titleW) / 2 + titleW, (int)(sh * 0.2f + titleSize), (int)(24 * scale), GREEN);

    if (DrawButton("PLAY", 0.0f)) {
        engine->currentState = GameState::WorldSelect;
        RefreshSaveList();
    }

    if (DrawButton("SETTINGS", 0.12f)) {
        engine->currentState = GameState::Settings;
        currentSettingsTab = SettingsTab::None;
    }

    if (DrawButton("EXIT", 0.24f)) {
        engine->CloseApp();
    }
}
void MenuSystem::DrawSettings() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = engine->GetUIScale();
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));

    DrawText("SETTINGS", (sw - MeasureText("SETTINGS", (int)(50 * scale))) / 2, (int)(sh * 0.1f), (int)(50 * scale), WHITE);

    if (currentSettingsTab == SettingsTab::None) DrawSettingsMain();
    else if (currentSettingsTab == SettingsTab::Graphics) DrawSettingsGraphics();
    else if (currentSettingsTab == SettingsTab::Advanced) DrawSettingsAdvanced();
}

void MenuSystem::DrawSettingsMain() {
    if (DrawButton("GRAPHICS", -0.1f)) currentSettingsTab = SettingsTab::Graphics;
    if (DrawButton("ADVANCED", 0.02f)) currentSettingsTab = SettingsTab::Advanced;
    if (DrawButton("CONTROLS", 0.14f)) { /* TODO */ }

    if (DrawButton("BACK", 0.30f)) engine->currentState = GameState::MainMenu;
}

void MenuSystem::DrawSettingsGraphics() {
    float dist = (float)engine->GetRenderDistance();
    if (DrawSlider("RENDER DISTANCE", 2, 32, &dist, -0.15f)) {
        engine->SetRenderDistance((int)dist);
    }

    float fov = engine->GetFOV();
    if (DrawSlider("FIELD OF VIEW", 30, 110, &fov, -0.05f, false)) {
        engine->SetFOV(fov);
    }

    const char* fsText = IsWindowFullscreen() ? "FULLSCREEN: ON" : "FULLSCREEN: OFF";
    if (DrawButton(fsText, 0.10f)) {
        engine->ToggleFullscreen();
    }

    if (DrawButton("BACK", 0.30f)) currentSettingsTab = SettingsTab::None;
}

void MenuSystem::DrawSettingsAdvanced() {
    const char* fpsText = engine->showSimpleFPS ? "SHOW FPS: ON" : "SHOW FPS: OFF";
    if (DrawButton(fpsText, -0.10f)) {
        engine->showSimpleFPS = !engine->showSimpleFPS;
    }

    if (DrawButton("BACK", 0.30f)) currentSettingsTab = SettingsTab::None;
}

void MenuSystem::DrawWorldSelect() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = engine->GetUIScale();
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));

    DrawText("SELECT WORLD", (sw - MeasureText("SELECT WORLD", (int)(50 * scale))) / 2, (int)(sh * 0.1f), (int)(50 * scale), WHITE);

    int startY = (int)(sh * 0.25f);
    int btnH = (int)(70 * scale);

    for (size_t i = 0; i < foundSaves.size(); i++) {
        int btnW = (int)(500 * scale);
        int x = (sw - btnW) / 2;
        int y = startY + (int)i * (btnH + 10);

        Rectangle rect = { (float)x, (float)y, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(GetMousePosition(), rect);

        DrawRectangleRec(rect, hover ? Fade(WHITE, 0.8f) : Fade(GRAY, 0.5f));
        DrawRectangleLinesEx(rect, 2 * scale, hover ? GREEN : LIGHTGRAY);

        if (foundSaves[i].hasCover) {
            Rectangle source = { 0, 0, (float)foundSaves[i].cover.width, (float)foundSaves[i].cover.height };
            Rectangle dest = { (float)x + 5 * scale, (float)y + 5 * scale, 120 * scale, (float)btnH - 10 * scale };
            Vector2 origin = { 0, 0 };
            DrawTexturePro(foundSaves[i].cover, source, dest, origin, 0.0f, WHITE);
            DrawText(foundSaves[i].name.c_str(), (int)(x + 140 * scale), (int)(y + 20 * scale), (int)(24 * scale), hover ? BLACK : WHITE);
        }
        else {
            DrawRectangle((int)(x + 5 * scale), (int)(y + 5 * scale), (int)(120 * scale), (int)(btnH - 10 * scale), BLACK);
            DrawText(foundSaves[i].name.c_str(), (int)(x + 140 * scale), (int)(y + 20 * scale), (int)(24 * scale), hover ? BLACK : WHITE);
        }

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            engine->LoadWorld(foundSaves[i].name);
            DisableCursor();
        }
    }

    if (DrawButton("CREATE NEW WORLD", 0.30f)) {
        engine->currentState = GameState::CreateWorld;
        strcpy(inputWorldName, "New World");
        strcpy(inputSeed, std::to_string(GetRandomValue(0, 9999)).c_str());
        activeInputBox = 0;
    }
    if (DrawButton("BACK", 0.42f)) engine->currentState = GameState::MainMenu;
}

void MenuSystem::DrawCreateWorld() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = engine->GetUIScale();
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));

    DrawText("CREATE WORLD", (sw - MeasureText("CREATE WORLD", (int)(50 * scale))) / 2, (int)(sh * 0.1f), (int)(50 * scale), WHITE);

    DrawTextBox("World Name:", inputWorldName, 32, -0.1f, 0);
    DrawTextBox("Seed:", inputSeed, 32, 0.1f, 1);

    if (DrawButton("GENERATE", 0.25f)) {
        std::string safeName = GetUniqueWorldName(inputWorldName);
        std::string s(inputSeed);
        int seedInt = 0;
        try { seedInt = std::stoi(s); }
        catch (...) { seedInt = (int)std::hash<std::string>{}(s); }
        engine->worldGen.SetSeed(seedInt);
        engine->LoadWorld(safeName);
        DisableCursor();
    }

    if (DrawButton("CANCEL", 0.37f)) engine->currentState = GameState::WorldSelect;
}

void MenuSystem::DrawPauseMenu() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = engine->GetUIScale();
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.6f));

    DrawText("PAUSED", (sw - MeasureText("PAUSED", (int)(60 * scale))) / 2, (int)(sh * 0.3f), (int)(60 * scale), WHITE);

    if (DrawButton("RESUME", 0.0f)) {
        engine->currentState = GameState::Playing;
        DisableCursor();
    }

    if (DrawButton("SAVE & QUIT", 0.15f)) {
        engine->SaveWorld();
        engine->currentState = GameState::MainMenu;
        engine->UnloadWorld();
    }
}