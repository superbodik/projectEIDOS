#include "../MenuSystem.h"
#include "../UITheme.h"
#include "../../Core/EidosEngine.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <cmath>

namespace fs = std::filesystem;
void MenuSystem::DrawMainMenu() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = Scale();

    float mx = (sw > 0) ? ((GetMouseX() - sw * 0.5f) / (float)sw) : 0.0f;
    float my = (sh > 0) ? ((GetMouseY() - sh * 0.5f) / (float)sh) : 0.0f;

    DrawVignette();

    const char* title = "E I D O S";
    int titleSz = (int)(86 * s);
    int titleW = MeasureText(title, titleSz);
    int titleX = (sw - titleW) / 2 + (int)(mx * -26.0f * s);
    int titleY = (int)(sh * 0.16f) + (int)(my * -12.0f * s);

    DrawText(title, titleX + (int)(4 * s), titleY + (int)(5 * s), titleSz, Fade(BLACK, 0.75f));
    DrawText(title, titleX, titleY, titleSz, TEXT_MAIN);

    const char* sub = "CRC  EIDOS   .   v0.7  SURFACE";
    int subSz = (int)(17 * s);
    int subW = MeasureText(sub, subSz);
    DrawText(sub, (sw - subW) / 2 + (int)(mx * -14.0f * s),
        titleY + titleSz + (int)(6 * s), subSz, ACCENT);

    float bw = 320 * s, bh = 50 * s, gap = 12 * s;
    float bx = (sw - bw) * 0.5f + mx * -6.0f * s;
    float by = sh * 0.50f + my * -3.0f * s;

    if (Button({ bx, by, bw, bh }, "PLAY")) {
        engine->currentState = GameState::WorldSelect;
        RefreshSaveList();
    }
    if (Button({ bx, by + (bh + gap), bw, bh }, "SETTINGS")) {
        settingsFromPause = false;
        currentSettingsTab = SettingsTab::None;
        engine->currentState = GameState::Settings;
    }
    if (Button({ bx, by + (bh + gap) * 2, bw, bh }, "QUIT", true, DANGER)) {
        engine->CloseApp();
    }
}


void MenuSystem::DrawPauseMenu() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float s = Scale();

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.55f));

    Rectangle p = PanelRect(440, 400);
    DrawPanel(p, "PAUSED");

    int fs = (int)(15 * s);
    DrawText(engine->currentWorldName.c_str(), (int)(p.x + 22 * s),
        (int)(p.y + 52 * s), fs, TEXT_DIM);

    if (ButtonRow(p, 0, "RESUME")) {
        engine->currentState = GameState::Playing;
        DisableCursor();
    }
    if (ButtonRow(p, 1, "SETTINGS")) {
        settingsFromPause = true;
        currentSettingsTab = SettingsTab::None;
        engine->currentState = GameState::Settings;
    }
    if (ButtonRow(p, 2, "SAVE WORLD")) {
        engine->SaveWorld();
    }
    if (ButtonRow(p, 3, "SAVE & QUIT", true, DANGER)) {
        engine->SaveWorld();
        engine->UnloadWorld();
        engine->currentState = GameState::MainMenu;
        RefreshSaveList();
    }
}

