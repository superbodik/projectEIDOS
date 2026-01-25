#pragma once
#include "raylib.h"
#include "../World/BlockType.h"
#include "../Inventory/Inventory.h" 
#include <functional>
#include <cmath>

using BlockProvider = std::function<BlockType(int, int, int)>;

enum class GameMode {
    Survival,
    Creative,
    Spectator
};

class Player {
public:
    Vector3 position;
    Vector3 velocity;
    Camera3D camera;

    bool isGrounded;
    GameMode currentMode;

    int targetedBlockID = 0;
    Vector3 targetedBlockPos = { 0, 0, 0 };

    Inventory inventory;

    const float playerHeight = 1.8f;
    const float playerWidth = 0.6f; 

    Player();

    void Update(BlockProvider getBlock, float dt);
    void SpawnSafe(BlockProvider getBlock);
    void SetGameMode(int mode);

    bool AddItem(int blockID, int count) {
        return inventory.AddItem(blockID, count);
    }

    void ConsumeCurrentItem() {
        inventory.ConsumeSelectedItem();
    }

private:
    void UpdateSurvival(BlockProvider getBlock, float dt);
    void UpdateFlying(BlockProvider getBlock, float dt);

    void UpdateCameraData();
    void UpdateRaycast(BlockProvider getBlock);

    Vector3 GetMovementInput(bool flattenY);

    bool CheckCollision(BlockProvider getBlock, Vector3 pos);

    bool IsPassable(BlockType type);
};