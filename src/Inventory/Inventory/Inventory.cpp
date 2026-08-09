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

