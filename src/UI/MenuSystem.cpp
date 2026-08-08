#include "MenuSystem.h"
#include "UITheme.h"
#include "../Core/EidosEngine.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <cmath>

namespace fs = std::filesystem;

const Color MenuSystem::BG_PANEL = UI::BG_PANEL;
const Color MenuSystem::BG_DEEP = UI::BG_DEEP;
const Color MenuSystem::LINE_SOFT = UI::LINE_SOFT;
const Color MenuSystem::LINE_HARD = UI::LINE_HARD;
const Color MenuSystem::ACCENT = UI::ACCENT;
const Color MenuSystem::ACCENT_DIM = UI::ACCENT_DIM;
const Color MenuSystem::TEXT_MAIN = UI::TEXT_MAIN;
const Color MenuSystem::TEXT_DIM = UI::TEXT_DIM;
const Color MenuSystem::DANGER = UI::DANGER;

MenuSystem::MenuSystem(EidosEngine* ptr) : engine(ptr) { RefreshSaveList(); }
MenuSystem::~MenuSystem() { UnloadSaveTextures(); }

void MenuSystem::UnloadSaveTextures() {
    for (auto& s : foundSaves) if (s.hasCover) UnloadTexture(s.cover);
    foundSaves.clear();
}

void MenuSystem::RefreshSaveList() {
    UnloadSaveTextures();
    deleteConfirm = -1;
    std::string path = "saves";
    if (!fs::exists(path)) fs::create_directories(path);
    for (const auto& entry : fs::directory_iterator(path)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();
        if (name == "MenuPanorama") continue;
        std::string full = entry.path().string();
        if (!fs::exists(full + "/level.dat")) continue;
        SaveSlot slot; slot.name = name; slot.path = full; slot.hasCover = false;
        std::string img = full + "/cover.png";
        if (fs::exists(img)) {
            slot.cover = LoadTexture(img.c_str());
            if (slot.cover.id > 0) {
                SetTextureFilter(slot.cover, TEXTURE_FILTER_BILINEAR);
                slot.hasCover = true;
            }
        }
        foundSaves.push_back(slot);
    }
}

std::string MenuSystem::GetUniqueWorldName(const std::string& base) {
    std::string name = base;
    if (name.empty()) name = "World";
    int i = 1;
    while (fs::exists("saves/" + name)) name = base + " " + std::to_string(i++);
    return name;
}

float MenuSystem::Scale() const {
    float s = (float)GetScreenHeight() / 900.0f;
    return std::clamp(s, 0.72f, 2.1f);
}

void MenuSystem::Update() {
    if (engine->currentState != GameState::CreateWorld) return;
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125) {
            char* t = (activeInputBox == 0) ? inputWorldName : inputSeed;
            int len = (int)strlen(t);
            if (len < 31) { t[len] = (char)key; t[len + 1] = '\0'; }
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        char* t = (activeInputBox == 0) ? inputWorldName : inputSeed;
        int len = (int)strlen(t);
        if (len > 0) t[len - 1] = '\0';
    }
    if (IsKeyPressed(KEY_TAB)) activeInputBox = (activeInputBox == 0) ? 1 : 0;
}

void MenuSystem::Render() {
    switch (engine->currentState) {
    case GameState::MainMenu:    DrawMainMenu();    break;
    case GameState::Settings:    DrawSettings();    break;
    case GameState::WorldSelect: DrawWorldSelect(); break;
    case GameState::CreateWorld: DrawCreateWorld(); break;
    case GameState::Paused:      DrawPauseMenu();   break;
    default: break;
    }
}

void MenuSystem::DrawVignette() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    for (int y = 0; y < sh; y += 2) {
        float ny = (y - sh * 0.5f) / (sh * 0.5f);
        unsigned char a = (unsigned char)std::clamp(ny * ny * 170.0f, 0.0f, 170.0f);
        DrawRectangle(0, y, sw, 2, { 0, 0, 0, a });
    }
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.30f));
}

Rectangle MenuSystem::PanelRect(float wUnits, float hUnits) const {
    float s = Scale();
    float w = wUnits * s, h = hUnits * s;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (w > sw - 40.0f) w = (float)sw - 40.0f;
    if (h > sh - 40.0f) h = (float)sh - 40.0f;
    return { (sw - w) * 0.5f, (sh - h) * 0.5f, w, h };
}

void MenuSystem::DrawPanel(Rectangle r, const char* title) {
    float s = Scale();
    DrawRectangleRec({ r.x + 4 * s, r.y + 6 * s, r.width, r.height }, Fade(BLACK, 0.40f));
    DrawRectangleRec(r, BG_PANEL);
    DrawRectangleLinesEx(r, 2.0f, LINE_HARD);
    DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)(3 * s), ACCENT_DIM);

    if (title) {
        int fs = (int)(30 * s);
        DrawText(title, (int)(r.x + 22 * s), (int)(r.y + 16 * s), fs, TEXT_MAIN);
        float ly = r.y + 16 * s + fs + 12 * s;
        DrawLine((int)(r.x + 22 * s), (int)ly, (int)(r.x + r.width - 22 * s), (int)ly, LINE_SOFT);
    }
}

bool MenuSystem::Button(Rectangle r, const char* text, bool enabled, Color tint) {
    float s = Scale();
    Vector2 m = GetMousePosition();
    bool hover = enabled && CheckCollisionPointRec(m, r);
    bool held = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    Color accent = (tint.a > 0) ? tint : ACCENT;
    Color bg = !enabled ? Color{ 20, 18, 15, 200 }
        : held ? Color{ 58, 48, 34, 255 }
        : hover ? Color{ 44, 38, 28, 255 } : Color{ 30, 27, 22, 235 };

    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, hover ? 2.0f : 1.0f, !enabled ? LINE_SOFT : (hover ? accent : LINE_SOFT));

    if (hover && enabled) DrawRectangle((int)r.x, (int)r.y, (int)(4 * s), (int)r.height, accent);

    int fs = (int)(22 * s);
    int tw = MeasureText(text, fs);
    Color tc = !enabled ? Color{ 96, 88, 76, 255 } : (hover ? accent : TEXT_MAIN);
    DrawText(text, (int)(r.x + (r.width - tw) * 0.5f), (int)(r.y + (r.height - fs) * 0.5f), fs, tc);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool MenuSystem::ButtonRow(Rectangle panel, int index, const char* text, bool enabled, Color tint) {
    float s = Scale();
    float bh = 46 * s, gap = 10 * s;
    Rectangle r = { panel.x + 22 * s, panel.y + 74 * s + index * (bh + gap),
                    panel.width - 44 * s, bh };
    return Button(r, text, enabled, tint);
}

bool MenuSystem::Toggle(Rectangle panel, int index, const char* label, bool value) {
    float s = Scale();
    float bh = 46 * s, gap = 10 * s;
    Rectangle r = { panel.x + 22 * s, panel.y + 74 * s + index * (bh + gap),
                    panel.width - 44 * s, bh };

    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);

    DrawRectangleRec(r, hover ? Color{ 44, 38, 28, 255 } : Color{ 30, 27, 22, 235 });
    DrawRectangleLinesEx(r, hover ? 2.0f : 1.0f, hover ? ACCENT : LINE_SOFT);

    int fs = (int)(21 * s);
    DrawText(label, (int)(r.x + 16 * s), (int)(r.y + (r.height - fs) * 0.5f), fs,
        hover ? TEXT_MAIN : Color{ 208, 199, 184, 255 });

    float kw = 54 * s, kh = 26 * s;
    Rectangle knob = { r.x + r.width - kw - 16 * s, r.y + (r.height - kh) * 0.5f, kw, kh };
    DrawRectangleRounded(knob, 0.5f, 8, value ? ACCENT_DIM : Color{ 34, 30, 25, 255 });
    DrawRectangleRoundedLines(knob, 0.5f, 8, value ? ACCENT : LINE_SOFT);

    float dotR = kh * 0.36f;
    float dotX = value ? (knob.x + knob.width - dotR - 5 * s) : (knob.x + dotR + 5 * s);
    DrawCircle((int)dotX, (int)(knob.y + kh * 0.5f), dotR, value ? ACCENT : Color{ 112, 102, 88, 255 });

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool MenuSystem::Slider(Rectangle panel, int index, const char* label, float minVal, float maxVal,
    float* value, bool isInt, const char* suffix) {
    float s = Scale();
    float bh = 46 * s, gap = 10 * s;
    Rectangle box = { panel.x + 22 * s, panel.y + 74 * s + index * (bh + gap),
                      panel.width - 44 * s, bh };

    DrawRectangleRec(box, Color{ 24, 27, 34, 235 });
    DrawRectangleLinesEx(box, 1.0f, LINE_SOFT);

    int fs = (int)(19 * s);
    DrawText(label, (int)(box.x + 14 * s), (int)(box.y + 6 * s), fs, Color{ 190, 200, 214, 255 });

    std::string val = isInt ? TextFormat("%d%s", (int)*value, suffix)
        : TextFormat("%.0f%s", *value, suffix);
    int vw = MeasureText(val.c_str(), fs);
    DrawText(val.c_str(), (int)(box.x + box.width - vw - 14 * s), (int)(box.y + 6 * s), fs, ACCENT);

    Rectangle track = { box.x + 14 * s, box.y + box.height - 15 * s, box.width - 28 * s, 6 * s };
    DrawRectangleRec(track, Color{ 16, 18, 24, 255 });

    float norm = std::clamp((*value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
    DrawRectangleRec({ track.x, track.y, track.width * norm, track.height }, ACCENT_DIM);

    float knobX = track.x + track.width * norm;
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, box);
    DrawCircle((int)knobX, (int)(track.y + track.height * 0.5f), 7 * s, hover ? ACCENT : LINE_HARD);

    if (hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float nx = std::clamp((m.x - track.x) / track.width, 0.0f, 1.0f);
        float nv = minVal + nx * (maxVal - minVal);
        if (isInt) nv = (float)(int)(nv + 0.5f);
        if (fabsf(nv - *value) > 0.001f) { *value = nv; return true; }
    }
    return false;
}

void MenuSystem::TextField(Rectangle panel, int index, const char* label, char* buf, int id) {
    float s = Scale();
    float bh = 46 * s, gap = 10 * s;
    Rectangle box = { panel.x + 22 * s, panel.y + 74 * s + index * (bh + gap),
                      panel.width - 44 * s, bh };

    bool active = (activeInputBox == id);
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, box);

    DrawRectangleRec(box, active ? Color{ 30, 40, 54, 255 } : Color{ 24, 27, 34, 235 });
    DrawRectangleLinesEx(box, active ? 2.0f : 1.0f, active ? ACCENT : (hover ? LINE_HARD : LINE_SOFT));

    int lfs = (int)(15 * s);
    DrawText(label, (int)(box.x + 12 * s), (int)(box.y + 5 * s), lfs, TEXT_DIM);

    int fs = (int)(21 * s);
    DrawText(buf, (int)(box.x + 12 * s), (int)(box.y + box.height - fs - 7 * s), fs, TEXT_MAIN);

    if (active && ((int)(GetTime() * 2.0f) % 2 == 0)) {
        int tw = MeasureText(buf, fs);
        DrawRectangle((int)(box.x + 14 * s + tw), (int)(box.y + box.height - fs - 7 * s), (int)(2 * s), fs, ACCENT);
    }

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activeInputBox = id;
}

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

void MenuSystem::DrawWorldSelect() {
    DrawVignette();
    float s = Scale();
    int sw = GetScreenWidth();

    Rectangle p = PanelRect(640, 620);
    DrawPanel(p, "SELECT WORLD");

    float listTop = p.y + 74 * s;
    float listBot = p.y + p.height - 128 * s;
    float visibleH = listBot - listTop;
    float rowH = 74 * s, rowGap = 8 * s;

    float totalH = (float)foundSaves.size() * (rowH + rowGap);
    float maxScroll = std::max(0.0f, totalH - visibleH);

    Vector2 m = GetMousePosition();
    Rectangle listRect = { p.x, listTop, p.width, visibleH };
    if (CheckCollisionPointRec(m, listRect))
        worldSelectScroll -= GetMouseWheelMove() * 44.0f * s;
    worldSelectScroll = std::clamp(worldSelectScroll, 0.0f, maxScroll);

    if (foundSaves.empty()) {
        const char* none = "No worlds yet.";
        int fs = (int)(20 * s);
        DrawText(none, (int)(p.x + (p.width - MeasureText(none, fs)) * 0.5f),
            (int)(listTop + 40 * s), fs, TEXT_DIM);
    }

    BeginScissorMode((int)p.x, (int)listTop, (int)p.width, (int)visibleH);

    for (size_t i = 0; i < foundSaves.size(); i++) {
        float ry = listTop + (float)i * (rowH + rowGap) - worldSelectScroll;
        if (ry + rowH < listTop - 4 || ry > listBot + 4) continue;

        Rectangle r = { p.x + 22 * s, ry, p.width - 44 * s, rowH };
        bool hover = CheckCollisionPointRec(m, r) && CheckCollisionPointRec(m, listRect);

        DrawRectangleRec(r, hover ? Color{ 38, 50, 66, 255 } : Color{ 26, 30, 38, 235 });
        DrawRectangleLinesEx(r, hover ? 2.0f : 1.0f, hover ? ACCENT : LINE_SOFT);

        float thumbW = 96 * s;
        Rectangle thumb = { r.x + 6 * s, r.y + 6 * s, thumbW, r.height - 12 * s };
        if (foundSaves[i].hasCover) {
            Rectangle src = { 0, 0, (float)foundSaves[i].cover.width, (float)foundSaves[i].cover.height };
            DrawTexturePro(foundSaves[i].cover, src, thumb, { 0, 0 }, 0, WHITE);
        }
        else {
            DrawRectangleRec(thumb, Color{ 16, 18, 24, 255 });
            const char* q = "?";
            int qs = (int)(26 * s);
            DrawText(q, (int)(thumb.x + (thumb.width - MeasureText(q, qs)) * 0.5f),
                (int)(thumb.y + (thumb.height - qs) * 0.5f), qs, LINE_SOFT);
        }
        DrawRectangleLinesEx(thumb, 1.0f, LINE_SOFT);

        int nfs = (int)(22 * s);
        DrawText(foundSaves[i].name.c_str(), (int)(thumb.x + thumbW + 14 * s),
            (int)(r.y + 16 * s), nfs, TEXT_MAIN);

        int sfs = (int)(15 * s);
        DrawText("click to load", (int)(thumb.x + thumbW + 14 * s),
            (int)(r.y + 16 * s + nfs + 6 * s), sfs, TEXT_DIM);

        float delW = 84 * s;
        Rectangle del = { r.x + r.width - delW - 8 * s, r.y + (r.height - 34 * s) * 0.5f, delW, 34 * s };
        bool delHover = CheckCollisionPointRec(m, del) && CheckCollisionPointRec(m, listRect);
        bool confirming = (deleteConfirm == (int)i);

        DrawRectangleRec(del, confirming ? Color{ 88, 32, 30, 255 }
            : (delHover ? Color{ 62, 34, 34, 255 } : Color{ 32, 26, 28, 235 }));
        DrawRectangleLinesEx(del, 1.0f, confirming ? DANGER : LINE_SOFT);
        int dfs = (int)(15 * s);
        const char* dtx = confirming ? "SURE?" : "DELETE";
        DrawText(dtx, (int)(del.x + (del.width - MeasureText(dtx, dfs)) * 0.5f),
            (int)(del.y + (del.height - dfs) * 0.5f), dfs, confirming ? TEXT_MAIN : DANGER);

        if (delHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (confirming) {
                std::error_code ec;
                fs::remove_all(foundSaves[i].path, ec);
                RefreshSaveList();
                EndScissorMode();
                return;
            }
            deleteConfirm = (int)i;
        }
        else if (hover && !delHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            std::string pick = foundSaves[i].name;
            EndScissorMode();
            engine->LoadWorld(pick);
            DisableCursor();
            return;
        }
    }

    EndScissorMode();

    if (maxScroll > 0.0f) {
        float sbH = visibleH * (visibleH / totalH);
        float sbY = listTop + (worldSelectScroll / maxScroll) * (visibleH - sbH);
        DrawRectangle((int)(p.x + p.width - 10 * s), (int)listTop, (int)(5 * s), (int)visibleH,
            Color{ 16, 18, 24, 200 });
        DrawRectangle((int)(p.x + p.width - 10 * s), (int)sbY, (int)(5 * s), (int)sbH, LINE_HARD);
    }

    float bh = 46 * s;
    Rectangle newBtn = { p.x + 22 * s, p.y + p.height - 116 * s, p.width - 44 * s, bh };
    Rectangle backBtn = { p.x + 22 * s, p.y + p.height - 62 * s, p.width - 44 * s, bh };

    if (Button(newBtn, "CREATE NEW WORLD")) {
        engine->currentState = GameState::CreateWorld;
        strcpy(inputWorldName, "New World");
        strcpy(inputSeed, std::to_string(GetRandomValue(0, 999999)).c_str());
        activeInputBox = 0;
    }
    if (Button(backBtn, "BACK")) engine->currentState = GameState::MainMenu;
    if (IsKeyPressed(KEY_ESCAPE)) engine->currentState = GameState::MainMenu;

    (void)sw;
}

void MenuSystem::DrawTextBoxed(const char* text, Rectangle box, int fontSize, Color col) {
    if (!text || fontSize < 1) return;

    std::string word, line;
    float y = box.y;
    float lineH = (float)fontSize + 5.0f;

    auto flush = [&]() {
        if (line.empty()) return;
        DrawText(line.c_str(), (int)box.x, (int)y, fontSize, col);
        y += lineH;
        line.clear();
        };

    for (const char* p = text; ; ++p) {
        if (*p && *p != ' ') { word += *p; continue; }

        std::string probe = line.empty() ? word : line + " " + word;
        if (MeasureText(probe.c_str(), fontSize) > box.width && !line.empty()) {
            flush();
            line = word;
        }
        else {
            line = probe;
        }
        word.clear();

        if (!*p) break;
        if (y + lineH > box.y + box.height) break;
    }
    flush();
}

void MenuSystem::DrawCreateWorld() {
    DrawVignette();
    float s = Scale();

    Rectangle p = PanelRect(560, 560);
    DrawPanel(p, "CREATE WORLD");

    TextField(p, 0, "WORLD NAME", inputWorldName, 0);
    TextField(p, 1, "SEED", inputSeed, 1);

    int fs = (int)(15 * s);
    float y = p.y + 74 * s + 2 * (56 * s) + 2 * s;

    DrawText("Leave the seed as text to hash it into a world.",
        (int)(p.x + 22 * s), (int)y, fs, TEXT_DIM);
    y += 28 * s;

    DrawText("DIFFICULTY", (int)(p.x + 22 * s), (int)y, (int)(13 * s), TEXT_DIM);
    y += 20 * s;

    float bw = (p.width - 44 * s - 16 * s) / 3.0f;
    float dh = 42 * s;
    for (int i = 0; i < 3; i++) {
        Rectangle r = { p.x + 22 * s + i * (bw + 8 * s), y, bw, dh };
        bool on = ((int)newWorldDifficulty == i);
        Color tint = (i == 0) ? Color{ 127, 168, 106, 255 }
            : (i == 1) ? ACCENT : DANGER;

        DrawRectangleRec(r, on ? Fade(tint, 0.22f) : BG_PANEL);
        DrawRectangleLinesEx(r, on ? 2.0f : 1.0f, on ? tint : LINE_HARD);

        const char* nm = WorldRules::Name((Difficulty)i);
        int tw = MeasureText(nm, (int)(16 * s));
        DrawText(nm, (int)(r.x + r.width * 0.5f) - tw / 2,
            (int)(r.y + r.height * 0.5f - 8 * s), (int)(16 * s),
            on ? tint : TEXT_DIM);

        if (CheckCollisionPointRec(GetMousePosition(), r) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            newWorldDifficulty = (Difficulty)i;
    }
    y += dh + 12 * s;

    const char* blurb = WorldRules::Blurb(newWorldDifficulty);
    DrawTextBoxed(blurb, { p.x + 22 * s, y, p.width - 44 * s, 60 * s },
        (int)(14 * s), TEXT_DIM);

    float bh = 46 * s;
    Rectangle gen = { p.x + 22 * s, p.y + p.height - 116 * s, p.width - 44 * s, bh };
    Rectangle cancel = { p.x + 22 * s, p.y + p.height - 62 * s, p.width - 44 * s, bh };

    if (Button(gen, "GENERATE WORLD")) {
        std::string safe = GetUniqueWorldName(inputWorldName);
        int seedInt = 0;
        try { seedInt = std::stoi(std::string(inputSeed)); }
        catch (...) { seedInt = (int)std::hash<std::string>{}(std::string(inputSeed)); }
        engine->worldGen.SetSeed(seedInt);
        engine->rules = WorldRules::Preset(newWorldDifficulty);
        engine->LoadWorld(safe);
        engine->rules = WorldRules::Preset(newWorldDifficulty);
        engine->SaveWorld(true);
        DisableCursor();
        return;
    }
    if (Button(cancel, "CANCEL")) {
        engine->currentState = GameState::WorldSelect;
        RefreshSaveList();
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        engine->currentState = GameState::WorldSelect;
        RefreshSaveList();
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
