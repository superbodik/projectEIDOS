#include "../EidosEngine.h"
#include <unordered_map>
#include "../CommandManager.h"
#include "../../Cinematic/Trailer.h"
#include "../../Cinematic/AutoShot.h"
#include "../../Inventory/BlockInfo.h"
#include "../../Inventory/MiningRules.h"
#include "../../Progression/QuestSystem.h"
#include "../../World/Chunk.h"
#include "rlgl.h"
#include <raymath.h>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <deque>
#include <chrono>

namespace fs = std::filesystem;

static int ProperMod(int a, int b) {
    return (a % b + b) % b;
}
void EidosEngine::Update() {
    if (IsWindowResized()) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        if (!IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) && !IsWindowMaximized()) {
            windowedW = screenWidth;
            windowedH = screenHeight;
        }
    }

    if (IsKeyPressed(KEY_F2)) CaptureScreenshot();
    if (IsKeyPressed(KEY_F3)) showSimpleFPS = !showSimpleFPS;
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    if (currentState != GameState::MainMenu) debugSystem.Update();

    if (currentState == GameState::Playing || currentState == GameState::Loading) {
        skySystem.Update(GetFrameTime(), player.position);
    }

    if ((trailer || autoShot) && currentState == GameState::MainMenu) {
        LoadWorld("CinematicScene");
        currentState = GameState::Playing;
        hudVisible = (autoShot != nullptr);
        return;
    }

    if (trailer && trailer->Active() && currentState == GameState::Playing) {
        float skyTime = skySystem.GetTime();
        trailer->Update(*this, player.camera, skyTime);
        skySystem.SetTime(skyTime);
        if (!trailer->Active()) CloseApp();
        UpdateChunks();
        return;
    }

    menuSystem->Update();

    if (autoShot && autoShot->Active()) autoShot->Update(*this);

    bool showCursor = (currentState != GameState::Playing && currentState != GameState::Loading) || debugSystem.IsConsoleOpen() || player.inventory.isOpen || questSystem->IsOpen();
    if (showCursor) { if (IsCursorHidden()) EnableCursor(); }
    else { if (!IsCursorHidden()) DisableCursor(); }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (debugSystem.IsConsoleOpen()) debugSystem.ToggleConsole();
        else if (questSystem->IsOpen()) questSystem->Toggle();
        else if (player.inventory.isOpen) player.inventory.Toggle();
        else if (currentState == GameState::Playing) currentState = GameState::Paused;
        else if (currentState == GameState::Paused) currentState = GameState::Playing;
    }
    player.camera.fovy = targetFOV;

    if (currentState == GameState::Playing) {
        autoSaveTimer += GetFrameTime();
        if (autoSaveTimer >= 60.0f) {
            SaveWorld(true);
            autoSaveTimer = 0.0f;
            debugSystem.Log("Auto-saved world data.");
        }

        if (!debugSystem.IsConsoleOpen()) {
            if (IsKeyPressed(KEY_E) && !questSystem->IsOpen()) player.inventory.Toggle();
            if (IsKeyPressed(KEY_L) && !player.inventory.isOpen) questSystem->Toggle();
            if (IsKeyPressed(KEY_F1)) hudVisible = !hudVisible;
            if (IsKeyPressed(KEY_F5)) player.CycleView();
        }

        questSystem->Update(*this, GetFrameTime());

        if (!debugSystem.IsConsoleOpen() && !player.inventory.isOpen && !questSystem->IsOpen()) {
            float dt = GetFrameTime();
            player.Update([this](int x, int y, int z) { return (BlockType)this->GetBlockAt(x, y, z); }, dt);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                player.TriggerHandSwing();

            bool used = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
                (TryEatHeldItem() || TryShapeClay() || TryMakePlanks() || TryMakeSticks() ||
                 TryMakeRope() || TryKnap() || TryAssembleTool());
            bool ate = used;

            auto markDirty = [this](int targetX, int targetZ) {
                int cx = (int)floor((float)targetX / 16.0f);
                int cz = (int)floor((float)targetZ / 16.0f);
                int lx = ProperMod(targetX, 16);
                int lz = ProperMod(targetZ, 16);

                std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
                for (auto& c : chunks) {
                    bool isTarget = (c->chunkX == cx && c->chunkZ == cz);
                    bool isNx = (lx == 0 && c->chunkX == cx - 1 && c->chunkZ == cz);
                    bool isPx = (lx == 15 && c->chunkX == cx + 1 && c->chunkZ == cz);
                    bool isNz = (lz == 0 && c->chunkX == cx && c->chunkZ == cz - 1);
                    bool isPz = (lz == 15 && c->chunkX == cx && c->chunkZ == cz + 1);
                    bool isNxNz = (lx == 0 && lz == 0 && c->chunkX == cx - 1 && c->chunkZ == cz - 1);
                    bool isPxNz = (lx == 15 && lz == 0 && c->chunkX == cx + 1 && c->chunkZ == cz - 1);
                    bool isNxPz = (lx == 0 && lz == 15 && c->chunkX == cx - 1 && c->chunkZ == cz + 1);
                    bool isPxPz = (lx == 15 && lz == 15 && c->chunkX == cx + 1 && c->chunkZ == cz + 1);

                    if (isTarget || isNx || isPx || isNz || isPz || isNxNz || isPxNz || isNxPz || isPxPz) {
                        c->dirty = true;
                    }
                }
                };

            // Breaking: held down, timed by rock/wood hardness and whether
            // the held tool matches. Stone-tier blocks are hard-gated - no
            // pickaxe means no progress at all, not just slower progress.
            if (!ate && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) {
                    int targetX = (int)floor(hit.x);
                    int targetY = (int)floor(hit.y);
                    int targetZ = (int)floor(hit.z);

                    if (!player.digActive || player.digX != targetX ||
                        player.digY != targetY || player.digZ != targetZ) {
                        player.digActive = true;
                        player.digX = targetX; player.digY = targetY; player.digZ = targetZ;
                        player.digProgress = 0.0f;
                    }

                    BlockType targetBlock = (BlockType)GetBlockAt(targetX, targetY, targetZ);
                    BlockType heldItem = (BlockType)player.inventory.GetSelectedBlockID();

                    if (player.currentMode == GameMode::Creative) {
                        player.digProgress = 1.0f;
                        player.digBlocked = false;
                    }
                    else {
                        float need = MiningRules::TimeToBreak(targetBlock, heldItem);
                        player.digBlocked = (need < 0.0f);
                        if (need < 0.0f) player.digProgress = 0.0f;
                        else if (need > 0.0f) player.digProgress += dt / need;
                        else player.digProgress = 1.0f;
                    }

                    if (player.digProgress >= 1.0f) {
                        int broken = (int)targetBlock;
                        GrantForage(broken, targetX, targetY, targetZ);

                        if (broken == (int)BlockType::BerryBushRipe) {
                            SetBlockGlobal(targetX, targetY, targetZ, (int)BlockType::BerryBush);
                        }
                        else {
                            if (broken != 0 && player.currentMode == GameMode::Survival)
                                player.inventory.AddItem(broken, 1);
                            SetBlockGlobal(targetX, targetY, targetZ, 0);
                        }

                        markDirty(targetX, targetZ);
                        player.digActive = false;
                        player.digProgress = 0.0f;
                    }
                }
                else {
                    player.digActive = false;
                    player.digProgress = 0.0f;
                }
            }
            else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                player.digActive = false;
                player.digProgress = 0.0f;
            }

            // Placing / using held item: still one action per click.
            if (!ate && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) {
                    int pX = (int)floor(player.position.x);
                    int pY = (int)floor(player.position.y);
                    int pZ = (int)floor(player.position.z);
                    if ((hit.px != pX || hit.pz != pZ) || (hit.py != pY && hit.py != pY + 1)) {
                        int id = player.inventory.GetSelectedBlockID();
                        if (id >= (int)BlockType::Berries) id = 0;
                        if (id != 0) {
                            SetBlockGlobal(hit.px, hit.py, hit.pz, id);
                            if (player.currentMode != GameMode::Creative) player.inventory.ConsumeSelectedItem();
                            markDirty(hit.px, hit.pz);
                        }
                    }
                }
            }
        }
        else {
            if (player.inventory.isOpen) player.inventory.UpdateInput();
            player.Update([this](int x, int y, int z) { return (BlockType)this->GetBlockAt(x, y, z); }, 0.0f);
        }
        UpdateChunks();
        UpdateWorldTicks(GetFrameTime());
        UpdateClimate(GetFrameTime());
        UpdateSurvival(GetFrameTime());
    }
    else if (currentState == GameState::Paused) {
    }
    else if (currentState == GameState::Loading) {
        UpdateChunks();
        Chunk* spawnChunk = GetChunkAt((int)player.position.x, (int)player.position.z);
        int waitRadius = 1;

        if (spawnChunk != nullptr && spawnChunk->state >= 3 && IsAreaLoaded(waitRadius)) {
            int h = worldGen.GetHeight((int)player.position.x, (int)player.position.z);
            float safeY = (float)std::max(h, 60) + 2.0f;
            if (player.position.y < safeY) player.position.y = safeY;
            player.velocity = { 0,0,0 };
            currentState = GameState::Playing;
        }
    }
    else {
        float time = (float)GetTime() * 0.05f;

        player.camera.position = { player.position.x, player.position.y + 35.0f, player.position.z };
        player.camera.target = {
            player.camera.position.x + sinf(time),
            player.camera.position.y + 35.0f,
            player.camera.position.z + cosf(time)
        };
        player.camera.up = { 0.0f, 1.0f, 0.0f };

        UpdateChunks();
    }

    UpdateWaterUniforms();
}

