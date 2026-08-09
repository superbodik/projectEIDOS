#pragma once
#include "raylib.h"
#include "../World/BlockType.h"
#include "../Inventory/Inventory.h"
#include "PlayerModel.h"
#include <functional>
#include <string>
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

    float bodyTempC = 36.6f;
    float warmthFromFire = 0.0f;

    static const int NUTRIENT_COUNT = 4;
    float nutrients[NUTRIENT_COUNT] = { 0.55f, 0.40f, 0.30f, 0.45f };

    float health = 1.0f;
    float maxHealth = 1.0f;
    float satiety = 0.85f;
    float hydration = 0.90f;
    float regenPool = 0.0f;
    bool  isSprinting = false;
    bool  isSwimming = false;

    const float playerHeight = 1.8f;
    const float playerWidth = 0.6f;

    std::string name = "Player";
    PlayerAppearance appearance;

    static const int VIEW_FIRST = 0;
    static const int VIEW_BACK = 1;
    static const int VIEW_FRONT = 2;
    int   viewMode = VIEW_FIRST;
    float thirdPersonDist = 3.4f;
    Vector3 smoothCam = { 0, 0, 0 };
    bool    smoothInit = false;

    float limbSwing = 0.0f;
    float swingAmount = 0.0f;
    float handSwing = 0.0f;

    int   digX = 0, digY = 0, digZ = 0;
    bool  digActive = false;
    bool  digBlocked = false;
    float digProgress = 0.0f;

    Vector3 EyePosition() const { return { position.x, position.y + 1.6f, position.z }; }
    Vector3 Forward() const;
    float   YawDegrees() const;
    float   PitchDegrees() const;
    void    CycleView() { viewMode = (viewMode + 1) % 3; smoothInit = false; }
    void    TriggerHandSwing() { handSwing = 1.0f; }

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
