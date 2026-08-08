#include "OverlayUI.h"

int OverlayUI::guiScaleSetting = 0;
#include <algorithm>
#include <cmath>
#include <rlgl.h>
#include <cmath>

void OverlayUI::DrawLoadingScreen(int screenWidth, int screenHeight, float progress,
    const char* worldName, const char* stage) {
    float s = std::clamp(screenHeight / 900.0f, 0.7f, 2.2f);

    DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 14, 12, 10, 255 });

    for (int y = 0; y < screenHeight; y += 2) {
        float ny = (y - screenHeight * 0.5f) / (screenHeight * 0.5f);
        unsigned char a = (unsigned char)std::clamp(ny * ny * 120.0f, 0.0f, 120.0f);
        DrawRectangle(0, y, screenWidth, 2, Color{ 0, 0, 0, a });
    }

    const char* title = "C R C   E I D O S";
    int ts = (int)(34 * s);
    int tw = MeasureText(title, ts);
    DrawText(title, (screenWidth - tw) / 2, (int)(screenHeight * 0.5f - 108 * s), ts,
        Color{ 236, 231, 221, 255 });

    if (worldName && worldName[0]) {
        int ns = (int)(17 * s);
        int nw = MeasureText(worldName, ns);
        DrawText(worldName, (screenWidth - nw) / 2,
            (int)(screenHeight * 0.5f - 62 * s), ns, Color{ 217, 164, 65, 255 });
    }

    float pct = std::clamp(progress, 0.0f, 1.0f);

    float barW = std::min(520.0f * s, screenWidth * 0.7f);
    float barH = 10.0f * s;
    float barX = (screenWidth - barW) * 0.5f;
    float barY = screenHeight * 0.5f;

    DrawRectangle((int)barX, (int)barY, (int)barW, (int)barH, Color{ 30, 27, 22, 255 });
    DrawRectangleLinesEx({ barX, barY, barW, barH }, 1.0f, Color{ 74, 63, 48, 255 });

    float fillW = (barW - 2.0f) * pct;
    if (fillW > 1.0f)
        DrawRectangle((int)barX + 1, (int)barY + 1, (int)fillW, (int)barH - 2,
            Color{ 217, 164, 65, 255 });

    if (pct < 1.0f && fillW > 2.0f) {
        float t = (float)GetTime() * 2.4f;
        float glow = 0.45f + 0.55f * (0.5f + 0.5f * sinf(t));
        DrawRectangle((int)(barX + fillW - 2.0f * s), (int)barY + 1,
            (int)(3 * s), (int)barH - 2,
            Color{ 255, 226, 160, (unsigned char)(255.0f * glow) });
    }

    const char* label = (stage && stage[0]) ? stage : "Generating terrain";
    int ls = (int)(15 * s);
    DrawText(label, (int)barX, (int)(barY + barH + 12 * s), ls, Color{ 179, 169, 152, 255 });

    const char* num = TextFormat("%d%%", (int)(pct * 100.0f));
    int nw2 = MeasureText(num, ls);
    DrawText(num, (int)(barX + barW) - nw2, (int)(barY + barH + 12 * s), ls,
        Color{ 217, 164, 65, 255 });

    int dots = ((int)(GetTime() * 3.0f)) % 4;
    float dotR = 2.5f * s;
    for (int i = 0; i < 3; i++) {
        Color c = (i < dots) ? Color{ 217, 164, 65, 255 } : Color{ 60, 52, 40, 255 };
        DrawCircle((int)(screenWidth * 0.5f + (i - 1) * 14 * s),
            (int)(barY + barH + 46 * s), dotR, c);
    }
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

    std::string comp;
    if (blockID == 130) comp = "Trace minerals";
    else if (blockID == 131) comp = "Native Copper (Cu)";
    else if (blockID == 132) comp = "Hematite (Fe2O3)";
    else if (blockID == 133) comp = "Bituminous Coal (C)";
    else if (blockID == 134) comp = "Native Gold (Au)";
    else if (blockID == 135) comp = "Kimberlite (C-diamond)";

    if (!comp.empty()) {
        panelH = 66;
        int cw = MeasureText(comp.c_str(), 16);
        if (cw + 80 > panelW) panelW = cw + 80;
        if (panelW < w + 80) panelW = w + 80;
    }

    DrawRectangle(px + 4, py + 4, panelW, panelH, Fade(BLACK, 0.3f));
    DrawRectangleGradientV(px, py, panelW, panelH, Fade(DARKGRAY, 0.95f), Fade(BLACK, 0.95f));
    DrawRectangleLinesEx({ (float)px, (float)py, (float)panelW, (float)panelH }, 2, Fade(LIGHTGRAY, 0.6f));

    DrawText(blockName.c_str(), px + 55, py + 8, 20, WHITE);
    if (!comp.empty()) {
        DrawText(comp.c_str(), px + 55, py + 30, 16, Fade(GREEN, 0.9f));
    }

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

HudLayout OverlayUI::ComputeLayout(int screenWidth, int screenHeight) {
    HudLayout L;

    int byH = screenHeight / 420;
    int byW = screenWidth / 700;
    int autoGui = (byH < byW) ? byH : byW;
    if (autoGui < 1) autoGui = 1;
    if (autoGui > 4) autoGui = 4;

    L.gui = (guiScaleSetting <= 0) ? autoGui : guiScaleSetting;
    if (L.gui < 1) L.gui = 1;
    if (L.gui > 4) L.gui = 4;

    L.slot = 20 * L.gui;
    L.pad = 2 * L.gui;
    L.hotbarW = 9 * (L.slot + L.pad) - L.pad;
    L.hotbarX = (screenWidth - L.hotbarW) / 2;
    L.hotbarY = screenHeight - L.slot - 10 * L.gui;

    L.iconScale = L.gui;
    L.iconStep = 8 * L.gui;
    L.rowW = L.iconStep * 10 - L.gui;

    return L;
}

static void DrawPixelIcon(const char* const* rows, int w, int h, int x, int y,
    int scale, Color col, float clipFrac) {
    int cut = (int)((float)w * clipFrac + 0.001f);
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            if (rows[r][c] != 'X') continue;
            if (c >= cut) continue;
            DrawRectangle(x + c * scale, y + r * scale, scale, scale, col);
        }
    }
}

static const char* HEART_ROWS[6] = {
    ".XX.XX.",
    "XXXXXXX",
    "XXXXXXX",
    ".XXXXX.",
    "..XXX..",
    "...X..."
};

static const char* FOOD_ROWS[7] = {
    "..XXX..",
    ".XXXXX.",
    "XXXXXXX",
    "XXXXXXX",
    ".XXXXX.",
    "..XXX..",
    "..X.X.."
};

static const char* DROP_ROWS[7] = {
    "...X...",
    "..XXX..",
    ".XXXXX.",
    "XXXXXXX",
    "XXXXXXX",
    ".XXXXX.",
    "..XXX.."
};

static void DrawIconRow(const char* const* rows, int w, int h, int x, int y,
    int scale, int step, int count, float frac,
    Color full, Color shell, bool rightToLeft) {
    float filled = frac * (float)count;

    for (int i = 0; i < count; i++) {
        int slot = rightToLeft ? (count - 1 - i) : i;
        int ix = x + slot * step;

        DrawPixelIcon(rows, w, h, ix + scale / 2 + 1, y + scale / 2 + 1, scale,
            Color{ 10, 11, 15, 175 }, 1.0f);
        DrawPixelIcon(rows, w, h, ix, y, scale, shell, 1.0f);

        float local = filled - (float)i;
        if (local <= 0.0f) continue;
        DrawPixelIcon(rows, w, h, ix, y, scale, full, local >= 1.0f ? 1.0f : local);
    }
}

void OverlayUI::DrawSurvivalHud(int screenWidth, int screenHeight, const SurvivalHud& s) {
    HudLayout L = ComputeLayout(screenWidth, screenHeight);

    int sc = L.iconScale;
    int iconH = 7 * sc;
    int gap = 3 * sc;

    int leftX = L.hotbarX;
    int rightX = L.hotbarX + L.hotbarW - L.rowW;

    int rowA = L.hotbarY - gap - iconH;
    int rowB = rowA - (iconH + gap);

    DrawIconRow(HEART_ROWS, 7, 6, leftX, rowA + sc, sc, L.iconStep, 10, s.health,
        Color{ 214, 58, 56, 255 }, Color{ 62, 22, 24, 215 }, false);

    DrawIconRow(FOOD_ROWS, 7, 7, rightX, rowA, sc, L.iconStep, 10, s.satiety,
        Color{ 198, 140, 62, 255 }, Color{ 56, 38, 20, 215 }, true);

    DrawIconRow(DROP_ROWS, 7, 7, rightX, rowB, sc, L.iconStep, 10, s.hydration,
        Color{ 82, 158, 226, 255 }, Color{ 22, 40, 60, 215 }, true);

    bool cold = s.bodyTempC < 35.4f;
    bool hot = s.bodyTempC > 38.0f;
    Color tempCol = cold ? Color{ 122, 186, 240, 255 }
        : hot ? Color{ 232, 120, 70, 255 } : Color{ 168, 198, 172, 255 };

    int tubeW = 3 * sc;
    int tubeH = iconH;
    int bulbR = 2 * sc;
    int tx = leftX;
    int ty = rowB;

    DrawRectangle(tx - sc, ty - sc, tubeW + sc * 2, tubeH + sc, Color{ 10, 11, 15, 175 });
    DrawCircle(tx + tubeW / 2, ty + tubeH, (float)(bulbR + sc), Color{ 10, 11, 15, 175 });

    float tFrac = (s.bodyTempC - 30.0f) / 12.0f;
    tFrac = tFrac < 0.0f ? 0.0f : (tFrac > 1.0f ? 1.0f : tFrac);
    int mercH = (int)((float)tubeH * tFrac);

    DrawRectangle(tx, ty + (tubeH - mercH), tubeW, mercH, tempCol);
    DrawCircle(tx + tubeW / 2, ty + tubeH, (float)bulbR, tempCol);

    int fs = 8 * sc;
    const char* tv = TextFormat("%.0f", s.bodyTempC);
    DrawText(tv, tx + tubeW + 4 * sc, ty + tubeH / 2 - fs / 2, fs, tempCol);

    if (cold || hot) {
        const char* w = cold ? "FREEZING" : "OVERHEATING";
        int wfs = 9 * sc;
        int ww = MeasureText(w, wfs);
        int wy = rowB - iconH - gap * 3;
        DrawRectangle(screenWidth / 2 - ww / 2 - 8 * sc, wy - 3 * sc,
            ww + 16 * sc, wfs + 6 * sc, Fade(BLACK, 0.5f));
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 4.0f);
        DrawText(w, screenWidth / 2 - ww / 2, wy, wfs,
            Fade(cold ? SKYBLUE : ORANGE, pulse));
    }
}
