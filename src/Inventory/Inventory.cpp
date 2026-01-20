#include "Inventory.h"
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
        AddItem(dragItem.id, dragItem.count);
        dragItem = { 0, 0 };
    }
}

Color Inventory::GetBlockColor(int id) {
    if (id == 1) return GREEN;
    if (id == 2) return BROWN;
    if (id == 3) return GRAY;
    if (id == 4) return BLACK;
    if (id == 5) return BEIGE;
    if (id == 6) return BLUE;
    if (id == 7) return WHITE;
    if (id == 8) return SKYBLUE;
    return MAGENTA;
}

void Inventory::UpdateInput() {
    if (isOpen) return; 

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

void Inventory::DrawSlot(int index, int x, int y, int size, bool isHotbar) {
    Rectangle rect = { (float)x, (float)y, (float)size, (float)size };

    if (isHotbar && index == currentSlotIndex) {
        DrawRectangleRec(rect, Fade(GOLD, 0.3f));
        DrawRectangleLinesEx(rect, 3, GOLD);
    }
    else {
        DrawRectangleRec(rect, Fade(LIGHTGRAY, 0.5f));
        DrawRectangleLinesEx(rect, 2, GRAY);
    }

    if (slots[index].id != 0) {
        DrawRectangle(x + 5, y + 5, size - 10, size - 10, GetBlockColor(slots[index].id));
        if (slots[index].count > 1) {
            DrawText(std::to_string(slots[index].count).c_str(), x + 2, y + 2, 10, WHITE);
        }
    }

    if (isOpen && CheckCollisionPointRec(GetMousePosition(), rect)) {
        DrawRectangleLinesEx(rect, 2, WHITE); 

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

        if (IsKeyPressed(KEY_Q)) {
            if (slots[index].id != 0) {
                slots[index].count--;
                if (slots[index].count <= 0) slots[index] = { 0, 0 };
            }
        }
    }
}

void Inventory::Draw(int sw, int sh) {
    if (!isOpen) return;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));

    int size = 50;
    int pad = 5;
    int startX = (sw - (9 * (size + pad))) / 2;
    int startY = sh / 2 - 100;

    for (int i = 9; i < 36; i++) {
        int r = (i - 9) / 9;
        int c = (i - 9) % 9;
        DrawSlot(i, startX + c * (size + pad), startY + r * (size + pad), size, false);
    }

    int hotbarY = startY + 4 * (size + pad) + 20;
    DrawText("Hotbar", startX, hotbarY - 20, 20, WHITE);
    for (int i = 0; i < 9; i++) {
        DrawSlot(i, startX + i * (size + pad), hotbarY, size, false);
    }

    if (dragItem.id != 0) {
        Vector2 m = GetMousePosition();
        DrawRectangle((int)m.x - 20, (int)m.y - 20, 40, 40, GetBlockColor(dragItem.id));
        DrawText(std::to_string(dragItem.count).c_str(), (int)m.x, (int)m.y, 10, WHITE);
    }
}

void Inventory::DrawHotbar(int sw, int sh, std::string(*nameResolver)(int)) {
    int size = 40;
    int pad = 5;
    int startX = (sw - (9 * (size + pad))) / 2;
    int startY = sh - size - 20;

    for (int i = 0; i < 9; i++) {
        DrawSlot(i, startX + i * (size + pad), startY, size, true);
    }

    if (slots[currentSlotIndex].id != 0 && nameResolver != nullptr) {
        std::string name = nameResolver(slots[currentSlotIndex].id);
        DrawText(name.c_str(), sw / 2 - MeasureText(name.c_str(), 20) / 2, startY - 25, 20, WHITE);
    }
}