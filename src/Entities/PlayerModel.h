#pragma once
#include "raylib.h"
#include <string>

enum class EquipSlot { Head, Chest, Legs, Feet, Count };
enum class CosmeticSlot { Hat, Cape, Trinket, Aura, Count };

struct PlayerAppearance {
    Color skin = { 214, 168, 128, 255 };
    Color shirt = { 96, 124, 154, 255 };
    Color trousers = { 74, 78, 96, 255 };
    Color boots = { 62, 52, 44, 255 };
    Color hair = { 86, 62, 40, 255 };

    int equipment[(int)EquipSlot::Count] = { 0, 0, 0, 0 };
    int cosmetics[(int)CosmeticSlot::Count] = { 0, 0, 0, 0 };

    bool HasEquip(EquipSlot s) const { return equipment[(int)s] != 0; }
    bool HasCosmetic(CosmeticSlot s) const { return cosmetics[(int)s] != 0; }
};

struct PlayerPose {
    Vector3 feet = { 0, 0, 0 };
    float   yaw = 0.0f;
    float   pitch = 0.0f;
    float   limbSwing = 0.0f;
    float   swingAmount = 0.0f;
    bool    sneaking = false;
    bool    swimming = false;
};

namespace PlayerModel {

    void Draw(const PlayerPose& pose, const PlayerAppearance& look,
        int heldBlockId, Texture2D atlas);

    void DrawPortrait(const PlayerAppearance& look, int heldBlockId,
        Texture2D atlas, Rectangle box, float spinDegrees);

    void BuildPortrait(const PlayerAppearance& look, int heldBlockId,
        Texture2D atlas, int width, int height, float spinDegrees);
    Texture2D PortraitTexture();
    bool PortraitReady();
    void ReleasePortrait();

    void DrawIsoBlock(int blockId, Texture2D atlas, float cx, float cy, float r);

    Color EquipTint(int itemId);
    const char* EquipSlotName(EquipSlot s);
    const char* CosmeticSlotName(CosmeticSlot s);
    bool FitsEquipSlot(int itemId, EquipSlot s);
    bool FitsCosmeticSlot(int itemId, CosmeticSlot s);

}
