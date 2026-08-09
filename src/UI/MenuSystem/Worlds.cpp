#include "../MenuSystem.h"
#include "../UITheme.h"
#include "../../Core/EidosEngine.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <cmath>

namespace fs = std::filesystem;
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

