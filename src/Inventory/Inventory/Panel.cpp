#include "../Inventory.h"
#include "../BlockInfo.h"
#include "../FoodSystem.h"
#include "../CreativeCatalog.h"
#include "../../UI/UITheme.h"
#include <algorithm>
#include "../../World/Chunk.h"
#include "../../UI/OverlayUI.h"
#include <cmath>
#include <string>
void Inventory::Draw(int sw, int sh, bool creativeMode) {
    if (!isOpen) return;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.72f));

    float uiScale;
    if (OverlayUI::guiScaleSetting > 0) {
        uiScale = 0.62f + 0.44f * (float)OverlayUI::guiScaleSetting;
    }
    else {
        uiScale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f), 0.62f, 2.2f);
    }
    float fitW = (float)sw / (214.0f + 14.0f + 9.0f * 55.0f + 32.0f + 62.0f + 40.0f);
    float fitH = (float)sh / 560.0f;
    uiScale = std::min(uiScale, std::min(fitW, fitH));
    uiScale = std::clamp(uiScale, 0.55f, 2.6f);
    auto S = [&](float v) { return (int)(v * uiScale + 0.5f); };

    int size = S(50);
    int pad = S(5);
    int gridW = 9 * (size + pad) - pad;
    int startX = (sw - gridW) / 2;
    int startY = sh / 2 - S(96);

    int panelPad = S(16);
    int dollW = S(168);
    int hItems = 4 * (size + pad) + S(40);
    int hEnv = S(372);
    int hCosm = S(336);

    int contentH = std::max(hItems, std::max(hEnv, hCosm));
    contentH = std::max(contentH, S(292));
    int panelH = contentH + S(34);
    int panelW = dollW + S(14) + gridW + panelPad * 2;

    int tabStrip = S(62);
    Rectangle panel = { (float)((sw - panelW - tabStrip) / 2), (float)(sh / 2 - panelH / 2),
                        (float)panelW, (float)panelH };

    DrawRectangleRec(panel, Color{ 22, 20, 17, 248 });
    DrawRectangleLinesEx(panel, 2, Color{ 74, 63, 48, 255 });

    Rectangle doll = { panel.x + S(12), panel.y + S(12),
                       (float)dollW, panel.height - S(24) };
    DrawPlayerPanel(doll);

    int bodyX = (int)(doll.x + doll.width) + S(14);
    int bodyY = (int)panel.y + S(12);
    int bodyW = (int)(panel.x + panel.width) - bodyX - S(12);

    DrawSideTabs(panel, uiScale);

    if (activeTab == Tab::Items) {
        int blockH = S(22) + 3 * (size + pad) + S(22) + (size + pad);
        int topPad = std::max(0, (contentH - blockH) / 2);

        startX = bodyX + (bodyW - gridW) / 2;
        startY = bodyY + topPad + S(22);

        DrawText("BACKPACK", startX, bodyY + topPad, S(15), UI::TEXT_DIM);
        for (int i = 9; i < 36; i++) {
            int r = (i - 9) / 9;
            int c = (i - 9) % 9;
            DrawSlot(i, startX + c * (size + pad), startY + r * (size + pad), size, false);
        }

        int hotbarY = startY + 3 * (size + pad) + S(22);
        DrawText("HOTBAR", startX, hotbarY - S(19), S(15), UI::TEXT_DIM);
        for (int i = 0; i < 9; i++) {
            DrawSlot(i, startX + i * (size + pad), hotbarY, size, true);
        }
    }
    else if (activeTab == Tab::Cosmetics) {
        DrawCosmeticsTab(bodyX, bodyY, bodyW, contentH);
    }
    else {
        DrawEnvironmentTab(bodyX, bodyY + 4, bodyW, contentH);
    }

    if (creativeMode && activeTab == Tab::Items) {
        DrawCreativeGrid(sw, sh);
    }

    if (dragItem.id != 0) {
        Vector2 m = GetMousePosition();
        DrawBlockIcon(dragItem.id, (int)m.x - 20, (int)m.y - 20, 40);
        DrawText(std::to_string(dragItem.count).c_str(), (int)m.x, (int)m.y, 10, WHITE);
    }
}

void Inventory::DrawCreativeGrid(int sw, int sh) {
    const int cols = 12;
    const int cellSize = 38;
    const int pad = 2;
    const int headerH = 22;

    int panelW = cols * (cellSize + pad) + 20;
    int panelX = sw - panelW - 10;
    int panelY = 30;
    int panelH = sh - 60;

    DrawRectangle(panelX - 5, panelY - 5, panelW + 10, panelH + 10, Color{ 18, 16, 14, 246 });
    DrawRectangleLinesEx({ (float)panelX - 5, (float)panelY - 5,
                           (float)panelW + 10, (float)panelH + 10 }, 2, Color{ 217, 164, 65, 255 });

    const auto& catalog = Creative::Catalog();
    const char* head = TextFormat("CREATIVE  -  %d blocks", Creative::TotalEntries());
    DrawText(head, panelX + 10, panelY + 2, 17, Color{ 217, 164, 65, 255 });

    int contentTop = panelY + 26;
    int viewH = panelH - 30;

    int totalH = 0;
    for (const auto& g : catalog) {
        int rows = ((int)g.entries.size() + cols - 1) / cols;
        totalH += headerH + rows * (cellSize + pad) + 6;
    }
    int maxScroll = std::max(0, totalH - viewH);
    if (creativeScroll > maxScroll) creativeScroll = maxScroll;
    if (creativeScroll < 0) creativeScroll = 0;

    BeginScissorMode(panelX - 5, contentTop, panelW + 10, viewH);

    int y = contentTop - creativeScroll;
    int hoveredID = 0;
    int hoveredBX = 0, hoveredBY = 0;

    for (const auto& g : catalog) {
        int rows = ((int)g.entries.size() + cols - 1) / cols;
        int blockH = headerH + rows * (cellSize + pad) + 6;

        if (y + blockH >= contentTop && y <= contentTop + viewH) {
            DrawText(g.name.c_str(), panelX + 10, y + 4, 13, Color{ 125, 114, 100, 255 });
            DrawRectangle(panelX + 10, y + headerH - 4, panelW - 20, 1, Color{ 58, 49, 38, 255 });

            for (size_t i = 0; i < g.entries.size(); i++) {
                int col = (int)i % cols;
                int row = (int)i / cols;
                int bx = panelX + 10 + col * (cellSize + pad);
                int by = y + headerH + row * (cellSize + pad);

                if (by + cellSize < contentTop || by > contentTop + viewH) continue;

                Rectangle rect = { (float)bx, (float)by, (float)cellSize, (float)cellSize };
                bool hover = CheckCollisionPointRec(GetMousePosition(), rect);

                DrawRectangleRec(rect, Color{ 34, 30, 25, 255 });
                DrawRectangleLinesEx(rect, hover ? 2.0f : 1.0f,
                    hover ? Color{ 217, 164, 65, 255 } : Color{ 58, 49, 38, 255 });
                DrawBlockIcon(g.entries[i].id, bx + 4, by + 4, cellSize - 8);

                if (hover) {
                    hoveredID = g.entries[i].id;
                    hoveredBX = bx;
                    hoveredBY = by;

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                        dragItem = { hoveredID, 64 };
                    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                        dragItem = { hoveredID, 1 };
                    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
                        slots[currentSlotIndex] = { hoveredID, 64 };
                }
            }
        }

        y += blockH;
    }

    EndScissorMode();

    if (maxScroll > 0) {
        float frac = (float)viewH / (float)(viewH + maxScroll);
        int barH = std::max(24, (int)(viewH * frac));
        int barY = contentTop + (int)((viewH - barH) *
            ((float)creativeScroll / (float)maxScroll));
        DrawRectangle(panelX + panelW - 6, contentTop, 3, viewH, Color{ 34, 30, 25, 255 });
        DrawRectangle(panelX + panelW - 6, barY, 3, barH, Color{ 217, 164, 65, 200 });
    }

    if (hoveredID != 0) {
        std::string name = BlockInfo::GetName(hoveredID);
        const char* line = TextFormat("%s  #%d", name.c_str(), hoveredID);
        int textW = MeasureText(line, 17);
        int tipX = hoveredBX - textW - 22;
        int tipY = hoveredBY - 2;

        if (tipX < 4) tipX = hoveredBX + cellSize + 10;
        if (tipY < panelY) tipY = panelY + 2;
        if (tipY + 24 > sh) tipY = sh - 26;

        DrawRectangle(tipX - 6, tipY - 3, textW + 14, 24, Color{ 12, 11, 9, 244 });
        DrawRectangleLines(tipX - 6, tipY - 3, textW + 14, 24, Color{ 217, 164, 65, 140 });
        DrawText(line, tipX, tipY + 2, 17, Color{ 236, 231, 221, 255 });
    }
}

void Inventory::DrawHotbar(int sw, int sh, std::string(*nameResolver)(int)) {
    HudLayout L = OverlayUI::ComputeLayout(sw, sh);

    for (int i = 0; i < 9; i++) {
        DrawSlot(i, L.hotbarX + i * (L.slot + L.pad), L.hotbarY, L.slot, true);
    }

    if (slots[currentSlotIndex].id != 0 && nameResolver != nullptr) {
        std::string name = nameResolver(slots[currentSlotIndex].id);
        int fs = 9 * L.gui;
        int tw = MeasureText(name.c_str(), fs);
        int ty = L.hotbarY - fs - 6 * L.gui;
        DrawRectangle(sw / 2 - tw / 2 - 6 * L.gui, ty - 3 * L.gui,
            tw + 12 * L.gui, fs + 6 * L.gui, Fade(BLACK, 0.45f));
        DrawText(name.c_str(), sw / 2 - tw / 2, ty, fs, RAYWHITE);
    }
}

