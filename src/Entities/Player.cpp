#include "Player.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <raymath.h>

Player::Player() {
    position = Vector3{ 0.5f, 150.0f, 0.5f };
    velocity = Vector3{ 0.0f, 0.0f, 0.0f };
    isGrounded = false;
    currentMode = GameMode::Survival;
    targetedBlockID = 0;
    targetedBlockPos = { 0, 0, 0 };

    camera = { 0 };
    camera.position = position;
    camera.target = Vector3{ 0.0f, 150.0f, 1.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void Player::SetGameMode(int mode) {
    if (mode == 0) currentMode = GameMode::Survival;
    else if (mode == 1) currentMode = GameMode::Creative;
    else if (mode == 2) currentMode = GameMode::Spectator;
    velocity = Vector3{ 0, 0, 0 };
}

void Player::SpawnSafe(BlockProvider getBlock) {
    for (int y = 250; y > 0; y--) {
        int x = (int)floor(position.x);
        int z = (int)floor(position.z);

        BlockType block = getBlock(x, y, z);

        if (block != BlockType::Air && block != BlockType::Water) {
            position = Vector3{ position.x, (float)y + 2.0f, position.z };
            velocity = Vector3{ 0, 0, 0 };

            camera.position = Vector3{ position.x, position.y + 1.6f, position.z };
            return;
        }
    }
    position = Vector3{ position.x, 150.0f, position.z };
    velocity = Vector3{ 0, 0, 0 };
}

void Player::Update(BlockProvider getBlock, float dt) {
    inventory.UpdateInput();

    if (!inventory.isOpen) {
        if (currentMode == GameMode::Survival) UpdateSurvival(getBlock, dt);
        else UpdateFlying(getBlock, dt);

        UpdateRaycast(getBlock);
        UpdateCameraData();

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
            if (targetedBlockID != 0) {
                bool found = false;
                for (int i = 0; i < 9; i++) {
                    if (inventory.slots[i].id == targetedBlockID) {
                        inventory.currentSlotIndex = i;
                        found = true;
                        break;
                    }
                }
                if (!found && currentMode == GameMode::Creative) {
                    inventory.slots[inventory.currentSlotIndex] = { targetedBlockID, 64 };
                }
            }
        }
    }
}

void Player::UpdateRaycast(BlockProvider getBlock) {
    Vector3 origin = camera.position;
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    forward = Vector3Normalize(forward);

    float step = 0.05f;
    float maxDist = 5.0f;
    targetedBlockID = 0;

    for (float d = 0; d < maxDist; d += step) {
        Vector3 p = Vector3Add(origin, Vector3Scale(forward, d));
        int bx = (int)floor(p.x); int by = (int)floor(p.y); int bz = (int)floor(p.z);

        BlockType type = getBlock(bx, by, bz);
        if (type != BlockType::Air && type != BlockType::Water) {
            targetedBlockID = (int)type;
            targetedBlockPos = Vector3{ (float)bx, (float)by, (float)bz };
            return;
        }
    }
}

void Player::UpdateSurvival(BlockProvider getBlock, float dt) {
    float speed = 6.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) speed = 13.0f; 

    Vector3 moveDir = GetMovementInput(true);

    position.x += moveDir.x * speed * dt;
    if (CheckCollision(getBlock, position)) {
        position.x -= moveDir.x * speed * dt;
    }

    position.z += moveDir.z * speed * dt;
    if (CheckCollision(getBlock, position)) {
        position.z -= moveDir.z * speed * dt;
    }

    float gravity = 28.0f;
    velocity.y -= gravity * dt;

    if (IsKeyPressed(KEY_SPACE) && isGrounded) {
        velocity.y = 9.0f;
        isGrounded = false;
    }

    position.y += velocity.y * dt;

    if (CheckCollision(getBlock, position)) {
        position.y -= velocity.y * dt;

        if (velocity.y < 0) {
            isGrounded = true;
        }
        else if (velocity.y > 0) {
        }
        velocity.y = 0;
    }
    else {
        isGrounded = false;
    }

    if (position.y < -50) {
        position = Vector3{ 0.5f, 150.0f, 0.5f };
        velocity = Vector3{ 0,0,0 };
    }
}

void Player::UpdateFlying(BlockProvider getBlock, float dt) {
    float speed = 15.0f;
    if (currentMode == GameMode::Spectator) speed = 40.0f; 
    if (IsKeyDown(KEY_LEFT_SHIFT)) speed *= 2.0f;

    Vector3 moveDir = GetMovementInput(false);

    if (IsKeyDown(KEY_SPACE)) moveDir.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL)) moveDir.y -= 1.0f;

    Vector3 nextPos = Vector3Add(position, Vector3Scale(moveDir, speed * dt));

    if (currentMode == GameMode::Spectator) {
        position = nextPos;
    }
    else {
        position.x += moveDir.x * speed * dt;
        if (CheckCollision(getBlock, position)) position.x -= moveDir.x * speed * dt;

        position.y += moveDir.y * speed * dt;
        if (CheckCollision(getBlock, position)) position.y -= moveDir.y * speed * dt;

        position.z += moveDir.z * speed * dt;
        if (CheckCollision(getBlock, position)) position.z -= moveDir.z * speed * dt;
    }

    velocity = Vector3{ 0, 0, 0 };
    isGrounded = false;
}

Vector3 Player::GetMovementInput(bool flattenY) {
    Vector3 direction = { 0 };
    if (IsKeyDown(KEY_W)) direction.x += 1.0f;
    if (IsKeyDown(KEY_S)) direction.x -= 1.0f;
    if (IsKeyDown(KEY_A)) direction.y -= 1.0f;
    if (IsKeyDown(KEY_D)) direction.y += 1.0f;

    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    if (flattenY) forward.y = 0;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3CrossProduct(forward, camera.up);
    right = Vector3Normalize(right);

    Vector3 moveDir = Vector3Add(Vector3Scale(forward, direction.x), Vector3Scale(right, direction.y));

    if (Vector3Length(moveDir) > 0) moveDir = Vector3Normalize(moveDir);
    return moveDir;
}

bool Player::IsPassable(BlockType type) {
    return type == BlockType::Air ||
        type == BlockType::Water ||
        type == BlockType::TallGrass ||
        type == BlockType::Rose ||
        type == BlockType::Dandelion ||
        type == BlockType::DeadBush;
}

bool Player::CheckCollision(BlockProvider getBlock, Vector3 pos) {
    float w = playerWidth / 2.0f;
    float h = playerHeight;

    int minX = (int)floor(pos.x - w);
    int maxX = (int)floor(pos.x + w);
    int minZ = (int)floor(pos.z - w);
    int maxZ = (int)floor(pos.z + w);
    int minY = (int)floor(pos.y);
    int maxY = (int)floor(pos.y + h - 0.01f);

    for (int x = minX; x <= maxX; x++) {
        for (int z = minZ; z <= maxZ; z++) {
            for (int y = minY; y <= maxY; y++) {
                BlockType block = getBlock(x, y, z);
                if (!IsPassable(block)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Player::UpdateCameraData() {
    UpdateCameraPro(&camera, Vector3{ 0,0,0 }, Vector3{ GetMouseDelta().x * 0.1f, GetMouseDelta().y * 0.1f, 0.0f }, 0.0f);

    Vector3 fwd = Vector3Subtract(camera.target, camera.position);
    fwd = Vector3Normalize(fwd);

    camera.position = Vector3{ position.x, position.y + 1.6f, position.z };
    camera.target = Vector3Add(camera.position, fwd);
}