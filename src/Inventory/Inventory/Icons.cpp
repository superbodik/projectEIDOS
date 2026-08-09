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

void Inventory::DrawCraftSlot(ItemStack& stack, int x, int y, int size) {
    Rectangle rect = { (float)x, (float)y, (float)size, (float)size };
    bool hovered = isOpen && CheckCollisionPointRec(GetMousePosition(), rect);

    DrawRectangleRec(rect, UI::BG_SLOT);
    DrawRectangleLinesEx(rect, 1.0f, hovered ? UI::ACCENT_DIM : UI::LINE_SOFT);

    if (stack.id != 0) {
        int inset = std::max(3, size / 10);
        DrawBlockIcon(stack.id, x + inset, y + inset, size - inset * 2);
        if (stack.count > 1) {
            const char* cnt = TextFormat("%d", stack.count);
            int cfs = std::max(9, size / 4);
            int cw = MeasureText(cnt, cfs);
            DrawText(cnt, x + size - cw - 3, y + size - cfs - 2, cfs, UI::TEXT_MAIN);
        }
    }

    if (hovered) {
        DrawRectangleLinesEx(rect, 2, UI::ACCENT);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (dragItem.id == stack.id && dragItem.id != 0) {
                int space = 64 - stack.count;
                int add = (dragItem.count < space) ? dragItem.count : space;
                stack.count += add;
                dragItem.count -= add;
                if (dragItem.count <= 0) dragItem = { 0,0 };
            }
            else {
                ItemStack temp = dragItem;
                dragItem = stack;
                stack = temp;
            }
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (dragItem.id == 0 && stack.id != 0) {
                int take = (stack.count + 1) / 2;
                dragItem = { stack.id, take };
                stack.count -= take;
                if (stack.count <= 0) stack = { 0,0 };
            }
            else if (dragItem.id != 0 && (stack.id == 0 || stack.id == dragItem.id) && stack.count < 64) {
                stack.id = dragItem.id;
                stack.count++;
                dragItem.count--;
                if (dragItem.count <= 0) dragItem = { 0,0 };
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

