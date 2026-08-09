#include "../MenuSystem.h"
#include "../UITheme.h"
#include "../../Core/EidosEngine.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <cmath>

namespace fs = std::filesystem;
void MenuSystem::ExitSettings() {
    if (settingsFromPause) engine->currentState = GameState::Paused;
    else engine->currentState = GameState::MainMenu;
    currentSettingsTab = SettingsTab::None;
}

void MenuSystem::DrawSettings() {
    DrawVignette();
    float s = Scale();

    Rectangle p = PanelRect(560, 520);
    const char* title = "SETTINGS";
    if (currentSettingsTab == SettingsTab::Graphics) title = "SETTINGS  /  GRAPHICS";
    else if (currentSettingsTab == SettingsTab::Advanced) title = "SETTINGS  /  GAMEPLAY";
    else if (currentSettingsTab == SettingsTab::Controls) title = "SETTINGS  /  CONTROLS";

    DrawPanel(p, title);

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (currentSettingsTab == SettingsTab::None) ExitSettings();
        else currentSettingsTab = SettingsTab::None;
        return;
    }

    if (currentSettingsTab == SettingsTab::None)          DrawSettingsMain(p);
    else if (currentSettingsTab == SettingsTab::Graphics) DrawSettingsGraphics(p);
    else if (currentSettingsTab == SettingsTab::Advanced) DrawSettingsAdvanced(p);
    else                                                   DrawSettingsControls(p);

    (void)s;
}

void MenuSystem::DrawSettingsMain(Rectangle p) {
    if (ButtonRow(p, 0, "GRAPHICS"))  currentSettingsTab = SettingsTab::Graphics;
    if (ButtonRow(p, 1, "GAMEPLAY"))  currentSettingsTab = SettingsTab::Advanced;
    if (ButtonRow(p, 2, "CONTROLS"))  currentSettingsTab = SettingsTab::Controls;

    float s = Scale();
    Rectangle back = { p.x + 22 * s, p.y + p.height - 62 * s, p.width - 44 * s, 46 * s };
    if (Button(back, "BACK")) ExitSettings();
}

void MenuSystem::DrawSettingsGraphics(Rectangle p) {
    float s = Scale();

    float dist = (float)engine->GetRenderDistance();
    if (Slider(p, 0, "RENDER DISTANCE", 2, 32, &dist, true, " chunks"))
        engine->SetRenderDistance((int)dist);

    float fov = engine->GetFOV();
    if (Slider(p, 1, "FIELD OF VIEW", 30, 110, &fov, false, " deg"))
        engine->SetFOV(fov);

    float fps = (float)engine->GetMaxFPS();
    if (Slider(p, 2, "FPS LIMIT  (0 = unlimited)", 0, 240, &fps, true, ""))
        engine->SetMaxFPS((int)fps);

    float gui = (float)OverlayUI::guiScaleSetting;
    if (Slider(p, 3, "GUI SCALE  (0 = auto)", 0, 3, &gui, true, "")) {
        OverlayUI::guiScaleSetting = std::clamp((int)gui, 0, 3);
        engine->SaveConfig();
    }
    {
        int g = OverlayUI::guiScaleSetting;
        const char* note = (g == 0) ? "Auto - follows the window size"
            : (g == 1) ? "Small"
            : (g == 2) ? "Medium - recommended" : "Large";
        DrawText(note, (int)(p.x + 24 * s),
            (int)(p.y + 74 * s + 4 * (56 * s) - 4 * s), (int)(14 * s), TEXT_DIM);
    }

    bool fullscreen = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
    if (Toggle(p, 4, "FULLSCREEN  (F11)", fullscreen)) engine->ToggleFullscreen();

    float bw = (p.width - 44 * s - 10 * s) * 0.5f;
    Rectangle apply = { p.x + 22 * s, p.y + p.height - 62 * s, bw, 46 * s };
    Rectangle back = { p.x + 22 * s + bw + 10 * s, p.y + p.height - 62 * s, bw, 46 * s };

    if (Button(apply, "APPLY", true, ACCENT)) {
        engine->SaveConfig();
        appliedFlash = 2.2f;
    }
    if (Button(back, "BACK")) currentSettingsTab = SettingsTab::None;

    if (appliedFlash > 0.0f) {
        appliedFlash -= GetFrameTime();
        const char* msg = "Settings saved";
        int fs2 = (int)(15 * s);
        int mw = MeasureText(msg, fs2);
        unsigned char a = (unsigned char)(255.0f * std::clamp(appliedFlash / 2.2f, 0.0f, 1.0f));
        DrawText(msg, (int)(p.x + (p.width - mw) * 0.5f),
            (int)(p.y + p.height - 86 * s), fs2, Color{ 127, 168, 106, a });
    }
}

void MenuSystem::DrawSettingsAdvanced(Rectangle p) {
    float s = Scale();

    if (Toggle(p, 0, "SHOW FPS COUNTER", engine->showSimpleFPS))
        engine->showSimpleFPS = !engine->showSimpleFPS;
    if (Toggle(p, 1, "SHOW WIND READOUT", engine->showWind))
        engine->showWind = !engine->showWind;
    if (Toggle(p, 2, "SHOW HUD  (F1)", engine->hudVisible))
        engine->hudVisible = !engine->hudVisible;

    Rectangle back = { p.x + 22 * s, p.y + p.height - 62 * s, p.width - 44 * s, 46 * s };
    if (Button(back, "BACK")) currentSettingsTab = SettingsTab::None;
}

void MenuSystem::DrawSettingsControls(Rectangle p) {
    float s = Scale();
    struct Bind { const char* key; const char* action; };
    static const Bind binds[] = {
        { "W A S D",      "Move" },
        { "SPACE",        "Jump" },
        { "SHIFT",        "Sprint" },
        { "E",            "Inventory" },
        { "L",            "Progression tree" },
        { "Q",            "Drop item" },
        { "1 - 9",        "Hotbar slot" },
        { "LMB / RMB",    "Break / place" },
        { "~",            "Console" },
        { "F1",           "Hide HUD" },
        { "F2",           "Screenshot" },
        { "F3",           "FPS counter" },
        { "F11",          "Fullscreen" },
    };

    int fs = (int)(18 * s);
    float y = p.y + 74 * s;
    float rowH = 26 * s;

    for (const Bind& b : binds) {
        if (y + rowH > p.y + p.height - 70 * s) break;
        DrawText(b.key, (int)(p.x + 30 * s), (int)y, fs, ACCENT);
        DrawText(b.action, (int)(p.x + 190 * s), (int)y, fs, Color{ 196, 204, 216, 255 });
        y += rowH;
    }

    Rectangle back = { p.x + 22 * s, p.y + p.height - 62 * s, p.width - 44 * s, 46 * s };
    if (Button(back, "BACK")) currentSettingsTab = SettingsTab::None;
}

