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
void Inventory::DrawSideTabs(Rectangle panel, float uiScale) {
    struct TabDef { const char* name; const char* hint; int glyphBlock; };
    static const TabDef TABS[TAB_COUNT] = {
        { "ITEMS",       "Backpack, hotbar and gear",        (int)BlockType::OakPlanks },
        { "COSMETICS",   "Skin decorations shown on you",    (int)BlockType::NativeGold },
        { "ENVIRONMENT", "Temperature, light and biome",     (int)BlockType::Ice },
    };

    const float tile = 54.0f * uiScale;
    const float gap = 6.0f * uiScale;
    float x = panel.x + panel.width + 8.0f * uiScale;
    float y = panel.y + 8.0f * uiScale;

    Vector2 m = GetMousePosition();
    int hovered = -1;

    for (int i = 0; i < TAB_COUNT; i++) {
        Rectangle r = { x, y + (float)i * (tile + gap), tile, tile };
        bool active = ((int)activeTab == i);
        bool hover = CheckCollisionPointRec(m, r);
        if (hover) hovered = i;

        Color bg = active ? Color{ 46, 40, 30, 252 }
            : (hover ? Color{ 34, 30, 24, 250 } : Color{ 22, 20, 17, 246 });
        DrawRectangleRec(r, bg);
        DrawRectangleLinesEx(r, active ? 2.0f : 1.0f,
            active ? Color{ 217, 164, 65, 255 } : Color{ 58, 49, 38, 255 });

        if (active)
            DrawRectangle((int)r.x - 3, (int)(r.y + tile * 0.12f), 3,
                (int)(tile * 0.76f), Color{ 217, 164, 65, 255 });

        DrawBlockIcon(TABS[i].glyphBlock, (int)(r.x + tile * 0.2f), (int)(r.y + tile * 0.2f), (int)(tile * 0.6f));

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) activeTab = (Tab)i;
    }

    if (hovered >= 0) {
        const char* title = TABS[hovered].name;
        const char* hint = TABS[hovered].hint;
        int hw = std::max(MeasureText(hint, 15), MeasureText(title, 15));
        float bx = x + tile + 10.0f;
        float by = y + (float)hovered * (tile + gap) + tile * 0.5f - 26.0f;
        if (bx + hw + 20.0f > (float)GetScreenWidth())
            bx = x - hw - 30.0f;
        DrawRectangle((int)bx, (int)by, hw + 20, 52, Color{ 12, 11, 9, 244 });
        DrawRectangleLines((int)bx, (int)by, hw + 20, 52, Color{ 217, 164, 65, 130 });
        DrawText(title, (int)bx + 10, (int)by + 7, 15, Color{ 217, 164, 65, 255 });
        DrawText(hint, (int)bx + 10, (int)by + 28, 15, Color{ 190, 182, 170, 255 });
    }
}

void Inventory::DrawGearSlot(int* target, Rectangle r, const char* label, bool cosmetic) {
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);
    bool filled = (target && *target != 0);

    Color edge = cosmetic ? Color{ 138, 108, 176, 255 } : Color{ 96, 82, 62, 255 };
    DrawRectangleRec(r, Color{ 17, 15, 13, 235 });
    DrawRectangleLinesEx(r, hover ? 2.0f : 1.0f, hover ? Color{ 217, 164, 65, 255 } : edge);

    if (filled) {
        DrawBlockIcon(*target, (int)r.x + 5, (int)r.y + 5, (int)r.width - 10);
    }
    else {
        int lw = MeasureText(label, 10);
        DrawText(label, (int)(r.x + r.width * 0.5f) - lw / 2,
            (int)(r.y + r.height * 0.5f) - 5, 10, Color{ 92, 83, 71, 255 });
    }

    if (!target || !hover || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    if (dragItem.id != 0) {
        int previous = *target;
        *target = dragItem.id;
        dragItem.count--;
        if (dragItem.count <= 0) dragItem = { 0, 0 };
        if (previous != 0) AddItem(previous, 1);
    }
    else if (filled) {
        if (!AddItem(*target, 1)) dragItem = { *target, 1 };
        *target = 0;
    }
}

void Inventory::DrawPlayerPanel(Rectangle box) {
    DrawRectangleRec(box, Color{ 16, 15, 13, 250 });
    DrawRectangleLinesEx(box, 1, Color{ 58, 49, 38, 255 });

    const char* name = (playerName && !playerName->empty()) ? playerName->c_str() : "Player";
    int nw = MeasureText(name, 18);
    DrawText(name, (int)(box.x + box.width * 0.5f) - nw / 2, (int)box.y + 10, 18,
        Color{ 236, 231, 221, 255 });
    DrawRectangle((int)box.x + 12, (int)box.y + 34, (int)box.width - 24, 1,
        Color{ 58, 49, 38, 255 });

    const float slot = 44.0f;
    float colX = box.x + 10.0f;
    float startY = box.y + 46.0f;
    float colH = box.height - 58.0f;
    float gap = (colH - 4.0f * slot) / 3.0f;
    if (gap < 6.0f) gap = 6.0f;

    Rectangle view = { colX + slot + 10.0f, startY,
                       box.width - slot - 30.0f, colH };

    if (PlayerModel::PortraitReady()) {
        Texture2D pt = PlayerModel::PortraitTexture();
        Rectangle src = { 0.0f, 0.0f, (float)pt.width, -(float)pt.height };
        DrawTexturePro(pt, src, view, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
    else if (appearance) {
        PlayerModel::DrawPortrait(*appearance, 0, Chunk::atlasTexture, view, portraitSpin);
    }
    DrawRectangleLinesEx(view, 1, Color{ 44, 38, 30, 255 });

    if (appearance) {
        static const char* LABELS[4] = { "HEAD", "CHEST", "LEGS", "FEET" };
        for (int i = 0; i < 4; i++) {
            Rectangle r = { colX, startY + (float)i * (slot + gap), slot, slot };
            DrawGearSlot(&appearance->equipment[i], r, LABELS[i], false);
        }
    }

}

void Inventory::DrawCosmeticsTab(int px, int py, int pw, int ph) {
    (void)ph;
    DrawText("SKIN DECORATIONS", px + 4, py + 4, 20, RAYWHITE);
    DrawText("Cosmetic only. Nothing here changes how tough you are.",
        px + 4, py + 30, 15, Color{ 148, 140, 128, 255 });

    if (!appearance) return;

    static const char* LABELS[4] = { "HAT", "CAPE", "TRINKET", "AURA" };
    static const char* NOTES[4] = {
        "Sits on top of your head.",
        "Hangs off your shoulders and sways as you walk.",
        "A small charm on your chest.",
        "Motes that orbit you. Visible to everyone.",
    };

    const float slot = 52.0f;
    int y = py + 64;

    for (int i = 0; i < 4; i++) {
        Rectangle r = { (float)(px + 6), (float)y, slot, slot };
        DrawGearSlot(&appearance->cosmetics[i], r, LABELS[i], true);

        DrawText(LABELS[i], px + 68, y + 6, 17, Color{ 214, 190, 236, 255 });
        DrawText(NOTES[i], px + 68, y + 27, 14, Color{ 140, 132, 122, 255 });

        y += (int)slot + 10;
    }

    DrawRectangle(px + 4, y + 4, pw - 8, 1, Color{ 58, 49, 38, 255 });
    DrawText("Drop any item into a slot to wear it. Right side shows it on your body.",
        px + 4, y + 16, 14, Color{ 120, 112, 102, 255 });
}

void Inventory::DrawNutritionTab(int px, int py, int pw, int ph) {
    (void)ph;
    static const char* groups[4] = { "Grain", "Vegetables", "Fruit", "Protein" };
    static const Color tones[4] = {
        { 196, 168, 88, 255 }, { 96, 152, 74, 255 },
        { 190, 96, 92, 255 }, { 168, 106, 76, 255 } };

    DrawText("BALANCED DIET", px + 4, py + 6, 20, RAYWHITE);
    DrawText("Living on one food group will not keep you healthy.",
        px + 4, py + 32, 15, Color{ 148, 156, 170, 255 });

    int barW = pw - 8;
    int y = py + 62;

    float hpVal = health ? *health : 1.0f;
    DrawStatBar(px + 4, y, barW, 26, hpVal, Color{ 190, 62, 60, 255 },
        "Health", TextFormat("%d%%", (int)(hpVal * 100.0f)));
    y += 32;

    float satVal = satiety ? *satiety : 0.0f;
    DrawStatBar(px + 4, y, barW, 26, satVal, Color{ 208, 150, 60, 255 },
        "Satiety", TextFormat("%d%%", (int)(satVal * 100.0f)));
    y += 32;

    float hydVal = hydration ? *hydration : 0.0f;
    DrawStatBar(px + 4, y, barW, 26, hydVal, Color{ 72, 148, 214, 255 },
        "Hydration", TextFormat("%d%%", (int)(hydVal * 100.0f)));
    y += 38;

    for (int i = 0; i < 4; i++) {
        float v = nutrients ? nutrients[i] : 0.0f;
        DrawStatBar(px + 4, y, barW, 24, v, tones[i], groups[i],
            TextFormat("%d%%", (int)(v * 100.0f)));
        y += 32;
    }

    y += 10;
    DrawRectangle(px + 4, y, barW, 1, Color{ 60, 66, 80, 255 });
    y += 12;

    DrawText("FORAGE  -  right-click to eat", px + 4, y, 16, Color{ 176, 184, 198, 255 });
    y += 24;

    for (const Food::Def& d : Food::All()) {
        int have = CountItem(d.id);
        bool poison = d.poison > 0.0f;
        Color nameCol = poison ? Color{ 198, 92, 88, 255 }
        : have > 0 ? RAYWHITE : Color{ 130, 138, 152, 255 };

        DrawBlockIcon(d.id, px + 6, y, 22);
        DrawText(d.name, px + 34, y + 4, 16, nameCol);

        const char* right = poison
            ? "POISON"
            : TextFormat("+%d%% %s", (int)(d.amount * 100.0f), Food::NutrientName(d.nutrient));
        int rw = MeasureText(right, 14);
        DrawText(right, px + pw - 12 - rw, y + 5, 14,
            poison ? Color{ 198, 92, 88, 255 } : tones[d.nutrient >= 0 ? d.nutrient : 0]);

        if (have > 0) {
            const char* cnt = TextFormat("x%d", have);
            DrawText(cnt, px + pw - 20 - rw - MeasureText(cnt, 14), y + 5, 14,
                Color{ 150, 158, 172, 255 });
        }
        y += 26;
    }
}

void Inventory::DrawEnvironmentTab(int px, int py, int pw, int ph) {
    (void)ph;
    int barW = pw - 8;
    int y = py;

    float airFrac = (env.airC + 40.0f) / 85.0f;
    Color airTone = (env.airC < 0.0f) ? Color{ 92, 150, 214, 255 }
        : (env.airC > 30.0f) ? Color{ 212, 108, 62, 255 } : Color{ 96, 168, 118, 255 };
    DrawStatBar(px + 4, y, barW, 26, airFrac, airTone, "Air temperature",
        TextFormat("%+.1f C", env.airC));
    y += 32;

    float feltFrac = (env.feltC + 40.0f) / 85.0f;
    Color feltTone = (env.feltC < 0.0f) ? Color{ 92, 150, 214, 255 }
        : (env.feltC > 30.0f) ? Color{ 212, 108, 62, 255 } : Color{ 96, 168, 118, 255 };
    DrawStatBar(px + 4, y, barW, 26, feltFrac, feltTone, "Feels like",
        TextFormat("%+.1f C", env.feltC));
    y += 32;

    bool cold = env.bodyC < 35.4f;
    bool hot = env.bodyC > 38.0f;
    float bodyFrac = (env.bodyC - 28.0f) / 15.0f;
    Color bodyTone = cold ? Color{ 92, 150, 214, 255 }
        : hot ? Color{ 212, 88, 62, 255 } : Color{ 96, 168, 118, 255 };
    const char* state = cold ? "  COLD" : (hot ? "  HOT" : "  OK");
    DrawStatBar(px + 4, y, barW, 26, bodyFrac, bodyTone, "Body",
        TextFormat("%.1f C%s", env.bodyC, state));
    y += 36;

    DrawRectangle(px + 4, y, barW, 1, Color{ 60, 66, 80, 255 });
    y += 9;

    struct Row { const char* k; std::string v; };
    Row rows[] = {
        { "Biome",    env.biome },
        { "Altitude", std::string(TextFormat("y = %d", env.altitude)) },
        { "Humidity", std::string(TextFormat("%d%%", (int)(env.humidity * 100.0f))) },
        { "Wind",     std::string(TextFormat("%.1f m/s", env.windSpeed)) },
        { "Time",     std::string(TextFormat("%02d:%02d",
                       (int)(env.timeOfDay * 24.0f) % 24,
                       (int)(env.timeOfDay * 1440.0f) % 60)) },
        { "Light",    std::string(TextFormat("sky %d / block %d", env.lightSky, env.lightBlock)) },
        { "Standing", env.underground ? std::string("underground") : std::string("on surface") },
    };

    for (const Row& r : rows) {
        DrawText(r.k, px + 8, y, 16, Color{ 142, 150, 164, 255 });
        int vw = MeasureText(r.v.c_str(), 16);
        DrawText(r.v.c_str(), px + pw - vw - 8, y, 16, RAYWHITE);
        y += 21;
    }

    y += 5;
    DrawRectangle(px + 4, y, barW, 1, Color{ 60, 66, 80, 255 });
    y += 9;
    DrawText("Temperature breakdown", px + 8, y, 15, Color{ 142, 150, 164, 255 });
    y += 20;

    struct BRow { const char* k; float v; };
    BRow brows[] = {
        { "latitude",  env.baseC },
        { "altitude",  env.altitudeC },
        { "day/night", env.diurnalC },
    };
    for (const BRow& b : brows) {
        DrawText(b.k, px + 8, y, 15, Color{ 178, 186, 200, 255 });
        std::string v = TextFormat("%+.1f C", b.v);
        int vw = MeasureText(v.c_str(), 15);
        DrawText(v.c_str(), px + pw - vw - 8, y, 15, Color{ 178, 186, 200, 255 });
        y += 18;
    }
}

