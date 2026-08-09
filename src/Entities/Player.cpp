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

    float planarSpeed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    float target = std::clamp(planarSpeed / 4.6f, 0.0f, 1.0f);
    swingAmount += (target - swingAmount) * std::min(1.0f, dt * 9.0f);
    limbSwing += planarSpeed * dt * 2.3f;
    if (limbSwing > 6.2831853f * 64.0f) limbSwing -= 6.2831853f * 64.0f;

    if (handSwing > 0.0f) handSwing = std::max(0.0f, handSwing - dt * 3.4f);
}

void Player::UpdateRaycast(BlockProvider getBlock) {
    Vector3 origin = EyePosition();
    Vector3 forward = Vector3Subtract(camera.target, origin);
    if (Vector3LengthSqr(forward) < 1e-6f) forward = { 0.0f, 0.0f, 1.0f };
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

    Vector3 wish = GetMovementInput(true);
    bool moving = (fabsf(wish.x) > 0.01f || fabsf(wish.z) > 0.01f);
    isSprinting = IsKeyDown(KEY_LEFT_SHIFT) && moving && satiety > 0.45f;

    if (isSprinting) speed = 11.0f;
    if (satiety < 0.25f) speed *= 0.85f;
    if (satiety < 0.10f) speed *= 0.82f;
    if (health < 0.25f) speed *= 0.80f;

    BlockType blockInFeet = getBlock((int)floor(position.x), (int)floor(position.y), (int)floor(position.z));
    BlockType blockInHead = getBlock((int)floor(position.x), (int)floor(position.y + playerHeight - 0.01f), (int)floor(position.z));

    auto isFoliage = [](BlockType t) {
        return t == BlockType::OakLeaves || t == BlockType::SpruceLeaves ||
            t == BlockType::BirchLeaves || t == BlockType::AcaciaLeaves ||
            t == BlockType::JungleLeaves;
        };

    auto isThicket = [](BlockType t) {
        return t == BlockType::BerryBush || t == BlockType::BerryBushRipe ||
            t == BlockType::CranberryBush;
        };

    if (isFoliage(blockInFeet) || isFoliage(blockInHead)) {
        speed *= 0.35f;
    }
    else if (isThicket(blockInFeet) || isThicket(blockInHead)) {
        speed *= 0.45f;
    }

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

    if ((isFoliage(blockInFeet) || isFoliage(blockInHead)) && velocity.y < 0.0f) {
        velocity.y *= 0.8f;
    }

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

    Vector3 forward = Vector3Subtract(camera.target, EyePosition());
    if (flattenY) forward.y = 0;
    if (Vector3LengthSqr(forward) < 1e-6f) forward = { 0.0f, 0.0f, 1.0f };
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
        type == BlockType::WaterSource ||
        type == BlockType::TallGrass ||
        type == BlockType::OakSapling ||
        type == BlockType::Fern ||
        type == BlockType::Reed ||
        type == BlockType::Torch ||
        type == BlockType::Rose ||
        type == BlockType::Dandelion ||
        type == BlockType::DeadBush ||
        type == BlockType::BrownMushroom ||
        type == BlockType::RedMushroom ||
        type == BlockType::OakLeaves ||
        type == BlockType::SpruceLeaves ||
        type == BlockType::BirchLeaves ||
        type == BlockType::AcaciaLeaves ||
        type == BlockType::JungleLeaves ||
        type == BlockType::SugarCane ||
        type == BlockType::BerryBush ||
        type == BlockType::BerryBushRipe ||
        type == BlockType::CranberryBush ||
        type == BlockType::Stick ||
        type == BlockType::StonePebble || type == BlockType::CopperPebble ||
        type == BlockType::IronPebble || type == BlockType::CoalPebble ||
        type == BlockType::GoldPebble || type == BlockType::DiamondPebble ||
        type == BlockType::FlintPebble || type == BlockType::GranitePebble ||
        type == BlockType::BasaltPebble || type == BlockType::LimestonePebble ||
        type == BlockType::SandstonePebble || type == BlockType::TinPebble ||
        type == BlockType::SilverPebble || type == BlockType::ZincPebble ||
        type == BlockType::LeadPebble || type == BlockType::BaritePebble ||
        type == BlockType::FluoritePebble || type == BlockType::PhosphoritePebble ||
        type == BlockType::PotashPebble || type == BlockType::TungstenPebble ||
        type == BlockType::UraniumPebble;
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

Vector3 Player::Forward() const {
    return Vector3Normalize(Vector3Subtract(camera.target, camera.position));
}

float Player::YawDegrees() const {
    Vector3 f = Forward();
    return atan2f(f.x, f.z) * RAD2DEG;
}

float Player::PitchDegrees() const {
    Vector3 f = Forward();
    return asinf(std::clamp(f.y, -1.0f, 1.0f)) * RAD2DEG;
}

void Player::UpdateCameraData() {
    UpdateCameraPro(&camera, Vector3{ 0,0,0 }, Vector3{ GetMouseDelta().x * 0.1f, GetMouseDelta().y * 0.1f, 0.0f }, 0.0f);

    Vector3 fwd = Vector3Subtract(camera.target, camera.position);
    fwd = Vector3Normalize(fwd);

    Vector3 eye = Vector3{ position.x, position.y + 1.6f, position.z };

    if (viewMode == VIEW_FIRST) {
        camera.position = eye;
        camera.target = Vector3Add(eye, fwd);
        return;
    }

    float yaw = atan2f(fwd.x, fwd.z);
    float pitch = std::clamp(asinf(std::clamp(fwd.y, -1.0f, 1.0f)),
        -35.0f * DEG2RAD, 45.0f * DEG2RAD);
    if (viewMode == VIEW_FRONT) {
        yaw += PI;
        pitch = -pitch;
    }

    float cp = cosf(pitch);
    Vector3 back = { -sinf(yaw) * cp, -sinf(pitch), -cosf(yaw) * cp };

    Vector3 wanted = Vector3Add(eye, Vector3Scale(back, thirdPersonDist));
    wanted.y += 0.30f;

    float dt = GetFrameTime();
    if (dt <= 0.0f || dt > 0.25f) dt = 1.0f / 60.0f;
    float k = 1.0f - expf(-14.0f * dt);

    if (!smoothInit) { smoothCam = wanted; smoothInit = true; }
    smoothCam = Vector3Lerp(smoothCam, wanted, k);

    camera.position = smoothCam;
    camera.target = Vector3Add(eye, Vector3Scale(fwd, 1.5f));
}
