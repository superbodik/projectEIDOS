#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct ItemStack {
    int id = 0;
    int count = 0;
};

class Inventory {
public:
    static const int INV_SIZE = 36;
    ItemStack slots[INV_SIZE];
    ItemStack dragItem;

    bool isOpen = false;
    int currentSlotIndex = 0;

    Inventory();

    bool AddItem(int id, int count);
    void RemoveOneFromHand();
    int GetSelectedBlockID() const;
    void ConsumeSelectedItem();
    void Toggle();

    void UpdateInput();
    void Draw(int screenW, int screenH);

    void DrawHotbar(int screenW, int screenH, std::string(*nameResolver)(int));

private:
    double lastClickTime = 0.0;
    int lastClickSlot = -1;

    void DrawSlot(int index, int x, int y, int size, bool isHotbar);
    void CollectItemsToHand(int targetID);
    Color GetBlockColor(int id);
};