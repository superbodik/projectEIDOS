#pragma once
#include "raylib.h"
#include "../Entities/PlayerModel.h"
#include <string>
#include <vector>
#include <functional>

struct ItemStack {
    int id = 0;
    int count = 0;
};

class Inventory {
public:
    static const int INV_SIZE = 36;
    ItemStack slots[INV_SIZE];
    ItemStack dragItem;

    enum class Tab { Items, Cosmetics, Environment };
    static const int TAB_COUNT = 3;

    bool isOpen = false;
    int currentSlotIndex = 0;
    bool showCreativePanel = false;
    int creativeScroll = 0;
    Tab activeTab = Tab::Items;

    struct EnvReadout {
        float airC = 15.0f;
        float feltC = 15.0f;
        float bodyC = 36.6f;
        float baseC = 15.0f;
        float altitudeC = 0.0f;
        float diurnalC = 0.0f;
        float humidity = 0.5f;
        float windSpeed = 0.0f;
        float timeOfDay = 0.5f;
        int   altitude = 64;
        int   lightSky = 15;
        int   lightBlock = 0;
        bool  underground = false;
        std::string biome = "Unknown";
    };
    EnvReadout env;

    const float* nutrients = nullptr;
    const float* satiety = nullptr;
    const float* hydration = nullptr;
    const float* health = nullptr;

    const std::string* playerName = nullptr;
    PlayerAppearance*  appearance = nullptr;

    Inventory();

    bool AddItem(int id, int count);
    int CountItem(int id) const {
        int total = 0;
        for (int i = 0; i < INV_SIZE; i++)
            if (slots[i].id == id) total += slots[i].count;
        return total;
    }
    void RemoveOneFromHand();
    int GetSelectedBlockID() const;
    void ConsumeSelectedItem();
    void Toggle();

    void UpdateInput();
    void Draw(int screenW, int screenH, bool creativeMode);

    void DrawHotbar(int screenW, int screenH, std::string(*nameResolver)(int));

private:
    double lastClickTime = 0.0;
    int lastClickSlot = -1;

    void DrawSlot(int index, int x, int y, int size, bool isHotbar);
    void CollectItemsToHand(int targetID);
    Color GetBlockColor(int id);
    static bool IsCubeBlock(int id);
    void DrawBlockIcon(int id, int x, int y, int size);

    float portraitSpin = 24.0f;
    bool  portraitDragging = false;
    float portraitDragX = 0.0f;

    void DrawSideTabs(Rectangle panel, float uiScale);
    void DrawPlayerPanel(Rectangle box);
    void DrawGearSlot(int* target, Rectangle r, const char* label, bool cosmetic);
    void DrawCosmeticsTab(int px, int py, int pw, int ph);
    void DrawNutritionTab(int px, int py, int pw, int ph);
    void DrawEnvironmentTab(int px, int py, int pw, int ph);
    void DrawStatBar(int x, int y, int w, int h, float frac, Color fill, const char* label, const char* value);
    void DrawCreativeGrid(int screenW, int screenH);
};
