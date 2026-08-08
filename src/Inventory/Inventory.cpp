#include "Inventory.h"
#include "BlockInfo.h"
#include "FoodSystem.h"
#include "CreativeCatalog.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include "../World/Chunk.h"
#include "../UI/OverlayUI.h"
#include <cmath>
#include <string>

Inventory::Inventory() {
    for (int i = 0; i < INV_SIZE; i++) slots[i] = { 0, 0 };
    dragItem = { 0, 0 };

    slots[0] = { 1, 64 };
    slots[1] = { 2, 64 };
    slots[2] = { 3, 64 };
}

bool Inventory::AddItem(int id, int count) {
    for (int i = 0; i < INV_SIZE; i++) {
        if (slots[i].id == id && slots[i].count < 64) {
            int space = 64 - slots[i].count;
            int add = (count < space) ? count : space;
            slots[i].count += add;
            count -= add;
            if (count <= 0) return true;
        }
    }

    for (int i = 0; i < INV_SIZE; i++) {
        if (slots[i].id == 0) {
            slots[i] = { id, count };
            return true;
        }
    }
    return false;
}

void Inventory::RemoveOneFromHand() {
    if (slots[currentSlotIndex].id != 0) {
        slots[currentSlotIndex].count--;
        if (slots[currentSlotIndex].count <= 0) slots[currentSlotIndex] = { 0, 0 };
    }
}

int Inventory::GetSelectedBlockID() const {
    return slots[currentSlotIndex].id;
}

void Inventory::ConsumeSelectedItem() {
    if (slots[currentSlotIndex].id != 0) {
        slots[currentSlotIndex].count--;
        if (slots[currentSlotIndex].count <= 0) slots[currentSlotIndex] = { 0, 0 };
    }
}

void Inventory::Toggle() {
    isOpen = !isOpen;
    if (!isOpen && dragItem.id != 0) {

        slots[currentSlotIndex] = { dragItem.id, 64 };
        dragItem = { 0, 0 };
    }
}

Color Inventory::GetBlockColor(int id) {
    switch (id) {
    case 0: return BLANK;
    case 1: case 2: return Color{ 28,100,200,255 };
    case 3: case 4: return ORANGE;
    case 5: return DARKGRAY;
    case 6: return Color{ 88,110,52,255 };
    case 7: return Color{ 96,74,54,255 };
    case 8: return Color{ 110,86,60,255 };
    case 9: return Color{ 70,56,42,255 };
    case 10: return Color{ 124,130,138,255 };
    case 11: return Color{ 196,180,146,255 };
    case 12: return Color{ 158,92,56,255 };
    case 13: return Color{ 122,116,107,255 };
    case 14: return Color{ 108,112,98,255 };
    case 15: return Color{ 98,93,83,255 };
    case 16: return Color{ 124,122,119,255 };
    case 17: return Color{ 104,102,99,255 };
    case 18: return Color{ 196,216,228,120 };
    case 20: return Color{ 212,208,178,255 };
    case 21: return Color{ 238,238,232,255 };
    case 22: return Color{ 78,78,88,255 };
    case 24: return Color{ 198,173,118,255 };
    case 25: return Color{ 172,88,58,255 };
    case 28: return Color{ 230,238,255,255 };
    case 30: return Color{ 168,128,118,255 };
    case 31: return Color{ 142,142,148,255 };
    case 32: return Color{ 28,28,32,255 };
    case 36: return Color{ 58,58,62,255 };
    case 37: return Color{ 118,128,142,255 };
    case 40: case 45: return Color{ 238,233,228,255 };
    case 41: return Color{ 83,88,93,255 };
    case 43: return Color{ 138,118,98,255 };
    case 44: return Color{ 228,223,213,255 };
    case 50: return Color{ 150,104,62,255 };
    case 53: return Color{ 138,82,58,255 };
    case 56: return Color{ 30,29,32,255 };
    case 60: return Color{ 186,158,66,255 };
    case 66: return Color{ 96,150,168,255 };
    case 100: return Color{ 104,76,46,255 };
    case 101: return Color{ 62,92,44,255 };
    case 102: return Color{ 66,50,34,255 };
    case 103: return Color{ 44,72,46,255 };
    case 104: return Color{ 198,194,182,255 };
    case 105: return Color{ 104,132,72,255 };
    case 106: return Color{ 94,62,38,255 };
    case 107: return Color{ 94,114,56,255 };
    case 108: return Color{ 104,76,46,255 };
    case 109: return Color{ 46,80,40,255 };
    case 110: return Color{ 58,96,52,255 };
    case 111: return Color{ 84,112,52,255 };
    case 112: return Color{ 116,94,54,255 };
    case 113: return Color{ 150,48,44,255 };
    case 114: return Color{ 186,168,62,255 };
    case 115: return Color{ 118,76,44,255 };
    case 116: return Color{ 158,60,50,255 };
    case 118: return Color{ 186,120,42,255 };
    case 119: return Color{ 28,138,28,255 };
    case 120: return Color{ 226,230,236,255 };
    case 121: return Color{ 150,186,208,200 };
    case 122: return Color{ 128,166,192,255 };
    case 123: return Color{ 62,142,46,255 };
    case 124: return Color{ 156,110,58,255 };
    case 125: return Color{ 44,116,38,255 };
    case 126: return Color{ 88,128,56,255 };
    case 127: return Color{ 236,178,74,255 };
    case 130: return GRAY;
    case 131: return Color{ 150,104,62,255 };
    case 132: return Color{ 138,82,58,255 };
    case 133: return Color{ 30,29,32,255 };
    case 134: return Color{ 186,158,66,255 };
    case 135: return Color{ 96,150,168,255 };
    case 136: return Color{ 62,60,64,255 };
    case 137: return Color{ 152,124,112,255 };
    case 138: return Color{ 44,44,48,255 };
    case 139: return Color{ 192,188,168,255 };
    case 140: return Color{ 186,166,122,255 };
    case 141: return Color{ 152,150,146,255 };
    case 142: return Color{ 196,198,204,255 };
    case 143: return Color{ 140,148,152,255 };
    case 180: return Color{ 225,222,208,255 };
    case 181: return Color{ 150,102,196,255 };
    case 182: return Color{ 138,124,96,255 };
    case 183: return Color{ 214,140,132,255 };
    case 184: return Color{ 58,46,40,255 };
    case 185: return Color{ 40,46,34,255 };
    case 190: return Color{ 108,112,118,255 };
    case 191: return Color{ 225,222,208,255 };
    case 192: return Color{ 150,102,196,255 };
    case 193: return Color{ 138,124,96,255 };
    case 194: return Color{ 214,140,132,255 };
    case 195: return Color{ 58,46,40,255 };
    case 196: return Color{ 40,46,34,255 };
    default:  return MAGENTA;
    }
}

void Inventory::UpdateInput() {
    if (isOpen) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            creativeScroll -= (int)wheel * 20;
            if (creativeScroll < 0) creativeScroll = 0;
        }
        return;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        currentSlotIndex -= (int)wheel;
        if (currentSlotIndex < 0) currentSlotIndex = 8;
        if (currentSlotIndex > 8) currentSlotIndex = 0;
    }

    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i)) currentSlotIndex = i;
    }

    if (IsKeyPressed(KEY_Q)) RemoveOneFromHand();
}

void Inventory::CollectItemsToHand(int targetID) {
    int maxStack = 64;
    for (int i = 0; i < INV_SIZE; i++) {
        if (dragItem.count >= maxStack) break;

        if (slots[i].id == targetID) {
            int space = maxStack - dragItem.count;
            int take = (slots[i].count <= space) ? slots[i].count : space;

            dragItem.count += take;
            slots[i].count -= take;

            if (slots[i].count <= 0) slots[i] = { 0, 0 };
        }
    }
}

bool Inventory::IsCubeBlock(int id) {
    if (id >= 100 && id <= 109) return true;
    if (id >= 5 && id <= 69) return true;
    if (id >= 120 && id <= 122) return true;
    if (id == 124) return true;
    return false;
}

void Inventory::DrawBlockIcon(int id, int x, int y, int size) {
    if (id == 0) return;

    if (Chunk::atlasTexture.id == 0) {
        DrawRectangle(x, y, size, size, GetBlockColor(id));
        return;
    }

    BlockType t = (BlockType)id;
    float u = 0.0f, v = 0.0f;
    Chunk::GetTextureUV(t, 2, u, v);

    float px = u * (float)Chunk::atlasTexture.width;
    float py = v * (float)Chunk::atlasTexture.height;
    float tile = Chunk::TileStep() * (float)Chunk::atlasTexture.width;

    Rectangle src = { px, py, tile, tile };
    Rectangle dst = { (float)x, (float)y, (float)size, (float)size };

    if (Chunk::IsWaterBlock(t) && Chunk::waterAtlas.id != 0) {
        float frameH = (float)Chunk::waterAtlas.height / (float)Chunk::WATER_FRAMES;
        Rectangle wsrc = { 0.0f, 0.0f, (float)Chunk::waterAtlas.width, frameH };
        DrawTexturePro(Chunk::waterAtlas, wsrc, dst, { 0, 0 }, 0.0f, WHITE);
        return;
    }

    if (IsCubeBlock(id)) {
        PlayerModel::DrawIsoBlock(id, Chunk::atlasTexture,
            (float)x + size * 0.5f, (float)y + size * 0.5f, size * 0.46f);
        return;
    }

    DrawTexturePro(Chunk::atlasTexture, src, dst, { 0, 0 }, 0.0f, WHITE);
}

void Inventory::DrawSlot(int index, int x, int y, int size, bool isHotbar) {
    Rectangle rect = { (float)x, (float)y, (float)size, (float)size };

    bool selected = isHotbar && index == currentSlotIndex;
    bool hovered = isOpen && CheckCollisionPointRec(GetMousePosition(), rect);

    DrawRectangleRec(rect, selected ? UI::BG_SLOT_HOT : UI::BG_SLOT);
    DrawRectangleLinesEx(rect, selected ? 2.0f : 1.0f,
        selected ? UI::ACCENT : (hovered ? UI::ACCENT_DIM : UI::LINE_SOFT));

    if (slots[index].id != 0) {
        int inset = std::max(3, size / 10);
        DrawBlockIcon(slots[index].id, x + inset, y + inset, size - inset * 2);
        if (slots[index].count > 1) {
            const char* cnt = TextFormat("%d", slots[index].count);
            int cfs = std::max(9, size / 4);
            int cw = MeasureText(cnt, cfs);
            DrawText(cnt, x + size - cw - 3, y + size - cfs - 2, cfs, UI::TEXT_MAIN);
        }
    }

    if (hovered) {
        DrawRectangleLinesEx(rect, 2, UI::ACCENT);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            double now = GetTime();
            bool doubleClick = (now - lastClickTime < 0.25) && (lastClickSlot == index);

            if (doubleClick) {
                if (dragItem.id == 0 && slots[index].id != 0) {
                    dragItem = slots[index];
                    slots[index] = { 0,0 };
                }
                if (dragItem.id != 0) CollectItemsToHand(dragItem.id);

                lastClickTime = 0.0;
                lastClickSlot = -1;
            }
            else {
                if (dragItem.id == slots[index].id && dragItem.id != 0) {
                    int space = 64 - slots[index].count;
                    int add = (dragItem.count < space) ? dragItem.count : space;
                    slots[index].count += add;
                    dragItem.count -= add;
                    if (dragItem.count <= 0) dragItem = { 0,0 };
                }
                else {
                    ItemStack temp = dragItem;
                    dragItem = slots[index];
                    slots[index] = temp;
                }
                lastClickTime = now;
                lastClickSlot = index;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (dragItem.id == 0 && slots[index].id != 0) {
                int take = (slots[index].count + 1) / 2;
                dragItem = { slots[index].id, take };
                slots[index].count -= take;
                if (slots[index].count <= 0) slots[index] = { 0,0 };
            }
            else if (dragItem.id != 0) {
                if (slots[index].id == 0 || slots[index].id == dragItem.id) {
                    if (slots[index].count < 64) {
                        slots[index].id = dragItem.id;
                        slots[index].count++;
                        dragItem.count--;
                        if (dragItem.count <= 0) dragItem = { 0,0 };
                    }
                }
            }
        }
    }
}

void Inventory::DrawStatBar(int x, int y, int w, int h, float frac, Color fill,
    const char* label, const char* value) {
    DrawRectangle(x, y, w, h, Color{ 26, 30, 38, 255 });
    DrawRectangleLines(x, y, w, h, Color{ 62, 70, 84, 255 });
    int fw = (int)((float)(w - 2) * (frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac)));
    DrawRectangle(x + 1, y + 1, fw, h - 2, fill);
    DrawText(label, x + 8, y + h / 2 - 8, 16, RAYWHITE);
    if (value) {
        int vw = MeasureText(value, 16);
        DrawText(value, x + w - vw - 8, y + h / 2 - 8, 16, RAYWHITE);
    }
}

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
