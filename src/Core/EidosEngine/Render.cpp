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

bool EidosEngine::IsCameraUnderwater() {
    if (currentState != GameState::Playing && currentState != GameState::Paused) return false;
    Vector3 cp = player.camera.position;
    BlockType at = GetBlockAt((int)floorf(cp.x), (int)floorf(cp.y + 0.08f), (int)floorf(cp.z));
    return Chunk::IsWaterBlock(at);
}

void EidosEngine::UpdateWaterUniforms() {
    cameraUnderwater = IsCameraUnderwater();

    float camPos[3] = { player.camera.position.x, player.camera.position.y, player.camera.position.z };
    float fogStartVal = (float)(renderDistance - 4) * Chunk::CHUNK_SIZE_X;
    float fogEndVal = (float)renderDistance * Chunk::CHUNK_SIZE_X;

    if (cinematicMode) {
        fogStartVal = (float)renderDistance * Chunk::CHUNK_SIZE_X * 1.15f;
        fogEndVal = (float)renderDistance * Chunk::CHUNK_SIZE_X * 2.6f;
    }

    Color fc = skySystem.GetFogColor();
    float fv[4] = { fc.r / 255.0f, fc.g / 255.0f, fc.b / 255.0f, 1.0f };

    if (cameraUnderwater) {
        fv[0] = 0.09f; fv[1] = 0.26f; fv[2] = 0.45f;
        fogStartVal = 2.0f;
        fogEndVal = 34.0f;
    }

    Vector3 sp = skySystem.GetSunPosition();
    Vector3 sd = Vector3Normalize(Vector3Subtract(sp, player.camera.position));
    float sunDir[3] = { sd.x, sd.y, sd.z };

    int biomeId = (int)worldGen.GetBiome((int)player.position.x, (int)player.position.z);
    windSystem.Update(GetFrameTime(), biomeId, 0.0f);
    Vector3 wv = windSystem.GetWindVector();
    float windStrength = sqrtf(wv.x * wv.x + wv.y * wv.y + wv.z * wv.z);

    if (fogShader.id > 0) {
        SetShaderValue(fogShader, fogViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(fogShader, fogColorLoc, fv, SHADER_UNIFORM_VEC4);
        SetShaderValue(fogShader, fogStartLoc, &fogStartVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(fogShader, fogEndLoc, &fogEndVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(fogShader, fogSunDirLoc, sunDir, SHADER_UNIFORM_VEC3);

        float windVec[3] = { wv.x, wv.y, wv.z };
        SetShaderValue(fogShader, fogWindVecLoc, windVec, SHADER_UNIFORM_VEC3);

        float windTime = (float)GetTime();
        SetShaderValue(fogShader, fogWindTimeLoc, &windTime, SHADER_UNIFORM_FLOAT);
    }

    if (waterShader.id > 0) {
        SetShaderValue(waterShader, waterViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(waterShader, waterFogColorLoc, fv, SHADER_UNIFORM_VEC4);
        SetShaderValue(waterShader, waterFogStartLoc, &fogStartVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(waterShader, waterFogEndLoc, &fogEndVal, SHADER_UNIFORM_FLOAT);

        float waterTime = (float)GetTime();
        SetShaderValue(waterShader, waterTimeLoc, &waterTime, SHADER_UNIFORM_FLOAT);

        float wsd[3] = { sd.x, std::max(sd.y, 0.05f), sd.z };
        SetShaderValue(waterShader, waterSunDirLoc, wsd, SHADER_UNIFORM_VEC3);

        SetShaderValue(waterShader, waterWindLoc, &windStrength, SHADER_UNIFORM_FLOAT);

        Color sc = skySystem.GetSkyColor();
        float sky[4] = { sc.r / 255.0f, sc.g / 255.0f, sc.b / 255.0f, 1.0f };
        SetShaderValue(waterShader, waterSkyTintLoc, sky, SHADER_UNIFORM_VEC4);

        int uw = cameraUnderwater ? 1 : 0;
        SetShaderValue(waterShader, waterUnderwaterLoc, &uw, SHADER_UNIFORM_INT);
    }
}

void EidosEngine::Render() {
    if (currentState == GameState::Playing && player.inventory.isOpen) {
        int held = player.inventory.GetSelectedBlockID();
        if (held >= (int)BlockType::Berries) held = 0;
        PlayerModel::BuildPortrait(player.appearance, held, Chunk::atlasTexture,
            256, 256, 22.0f + sinf((float)GetTime() * 0.5f) * 16.0f);
    }

    BeginDrawing();
    ClearBackground(skySystem.GetSkyColor());

    if (currentState == GameState::Loading) {

        const int waitRadius = 1;
        const int needed = (waitRadius * 2 + 1) * (waitRadius * 2 + 1);
        int ready = 0;
        int progressPoints = 0;

        int pcx = (int)floor(player.position.x / (float)Chunk::CHUNK_SIZE_X);
        int pcz = (int)floor(player.position.z / (float)Chunk::CHUNK_SIZE_Z);
        {
            std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
            for (int dx = -waitRadius; dx <= waitRadius; dx++) {
                for (int dz = -waitRadius; dz <= waitRadius; dz++) {
                    Chunk* c = GetChunkAt((pcx + dx) * Chunk::CHUNK_SIZE_X,
                        (pcz + dz) * Chunk::CHUNK_SIZE_Z);
                    if (!c) continue;
                    int st = c->state.load();
                    if (st > 3) st = 3;
                    progressPoints += st;
                    if (st >= 3) ready++;
                }
            }
        }

        {
            static double lastGate = 0.0;
            if (GetTime() - lastGate > 0.5) {
                lastGate = GetTime();
                Chunk* here = GetChunkAt((int)player.position.x, (int)player.position.z);
                printf("[load] player %.0f,%.0f,%.0f -> chunk %d,%d | under foot: %s "
                    "| ready %d/%d | points %d\n",
                    player.position.x, player.position.y, player.position.z,
                    pcx, pcz,
                    here ? ("state " + std::to_string(here->state.load())).c_str() : "MISSING",
                    ready, needed, progressPoints);
                fflush(stdout);
            }
        }

        float progress = (float)progressPoints / (float)(needed * 3);
        const char* stage = (progressPoints == 0) ? "Shaping the land"
            : (ready < needed / 2) ? "Carving caves and rivers"
            : (ready < needed) ? "Growing the surface"
            : "Stepping in";

        OverlayUI::DrawLoadingScreen(GetScreenWidth(), GetScreenHeight(), progress,
            currentWorldName.c_str(), stage);
        EndDrawing();
        return;
    }

    BeginMode3D(player.camera);

    bool isMenuState = (currentState == GameState::MainMenu || currentState == GameState::Settings || currentState == GameState::WorldSelect || currentState == GameState::CreateWorld);

    if (isMenuState && hasPanorama) {
        DrawPanorama();
    }
    else {
        skySystem.Draw(player.camera);

        skySystem.DrawClouds(windSystem.GetDirection(), windSystem.GetSpeed());
        skySystem.DrawMotes(player.camera.position, windSystem.GetDirection(), windSystem.GetSpeed());

        if (fogShader.id > 0) {
            BeginShaderMode(fogShader);
        }

        {
            std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
            for (auto& chunk : chunks) {
                int pChunkX = (int)floor(player.camera.position.x / Chunk::CHUNK_SIZE_X);
                int pChunkZ = (int)floor(player.camera.position.z / Chunk::CHUNK_SIZE_Z);
                if (abs(chunk->chunkX - pChunkX) <= renderDistance && abs(chunk->chunkZ - pChunkZ) <= renderDistance) {
                    if (chunk->hasMesh && IsChunkInFrustum(chunk.get())) {
                        chunk->Draw();
                    }
                }
            }
        }

        if (fogShader.id > 0) EndShaderMode();

        {
            rlEnableDepthTest();
            rlDisableDepthMask();
            BeginBlendMode(BLEND_ALPHA);

            std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
            int pChunkX = (int)floor(player.camera.position.x / Chunk::CHUNK_SIZE_X);
            int pChunkZ = (int)floor(player.camera.position.z / Chunk::CHUNK_SIZE_Z);
            for (auto& chunk : chunks) {
                if (abs(chunk->chunkX - pChunkX) <= renderDistance && abs(chunk->chunkZ - pChunkZ) <= renderDistance) {
                    if (chunk->hasMeshTransp && IsChunkInFrustum(chunk.get())) {
                        chunk->DrawTranslucent();
                    }
                }
            }
            for (auto& chunk : chunks) {
                if (abs(chunk->chunkX - pChunkX) <= renderDistance && abs(chunk->chunkZ - pChunkZ) <= renderDistance) {
                    if (chunk->hasMeshWater && IsChunkInFrustum(chunk.get())) {
                        chunk->DrawWater();
                    }
                }
            }

            EndBlendMode();
            rlEnableDepthMask();
        }

        if (currentState == GameState::Playing && hudVisible && !debugSystem.IsConsoleOpen() && !player.inventory.isOpen) {
            RayHitInfo hit = CastRay(5.0f);
            if (hit.hit) DrawCubeWires({ hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f }, 1.01f, 1.01f, 1.01f, BLACK);
        }

        if (currentState == GameState::Playing) {
            int held = player.inventory.GetSelectedBlockID();
            if (held >= (int)BlockType::Berries) held = 0;

            if (player.viewMode != Player::VIEW_FIRST) {
                PlayerPose pose;
                pose.feet = player.position;
                pose.yaw = player.YawDegrees();
                pose.pitch = player.PitchDegrees();
                pose.limbSwing = player.limbSwing;
                pose.swingAmount = player.swingAmount;
                pose.sneaking = IsKeyDown(KEY_LEFT_CONTROL);
                PlayerModel::Draw(pose, player.appearance, held, Chunk::atlasTexture);
            }
            else if (hudVisible) {
                DrawFirstPersonHands(held);
            }
        }

        if (debugSystem.showChunkBorders) {
            std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
            for (auto& chunk : chunks) {
                int pChunkX = (int)floor(player.camera.position.x / Chunk::CHUNK_SIZE_X);
                int pChunkZ = (int)floor(player.camera.position.z / Chunk::CHUNK_SIZE_Z);
                if (abs(chunk->chunkX - pChunkX) <= renderDistance && abs(chunk->chunkZ - pChunkZ) <= renderDistance) {
                    if (chunk->hasMesh || chunk->hasMeshTransp) {
                        Vector3 pos = { (float)chunk->chunkX * 16, 0, (float)chunk->chunkZ * 16 };
                        DrawCubeWiresV({ pos.x + 8, 128, pos.z + 8 }, { 16, 256, 16 }, YELLOW);
                    }
                }
            }
        }
    }
    EndMode3D();

    if (currentState == GameState::Playing && hudVisible) DrawPlayerNametag();

    if (!isMenuState && !cameraUnderwater) {
        Vector3 sp = skySystem.GetSunPosition();
        float sunH = std::clamp((sp.y - player.camera.position.y) / 400.0f, 0.0f, 1.0f);
        if (sunH > 0.03f) {
            Vector3 fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
            Vector3 toSun = Vector3Normalize(Vector3Subtract(sp, player.camera.position));
            float facing = Vector3DotProduct(fwd, toSun);
            if (facing > 0.25f) {
                float k = (facing - 0.25f) / 0.75f * sunH;
                Vector2 scr = GetWorldToScreen(sp, player.camera);
                BeginBlendMode(BLEND_ADDITIVE);
                DrawCircleGradient((int)scr.x, (int)scr.y, 260.0f * k + 40.0f,
                    Color{ 255, 238, 200, (unsigned char)(60.0f * k) }, BLANK);
                DrawCircleGradient((int)scr.x, (int)scr.y, 100.0f * k + 20.0f,
                    Color{ 255, 248, 226, (unsigned char)(85.0f * k) }, BLANK);
                EndBlendMode();
            }
        }
    }

    if (cameraUnderwater && !isMenuState) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 20, 60, 110, 70 });
    }

    bool drawHud = hudVisible || currentState != GameState::Playing;

    if (showSimpleFPS && drawHud) {
        DrawText(TextFormat("%d FPS", GetFPS()), 10, 10, 20, GREEN);
        if (showWind) {
            float playerYaw = atan2f(player.camera.target.z - player.camera.position.z,
                                      player.camera.target.x - player.camera.position.x);
            DrawText(windSystem.GetWindInfo(playerYaw).c_str(), 10, 30, 16, Fade(WHITE, 0.7f));
        }
    }

    if (currentState == GameState::Playing) {
        if (player.inventory.isOpen) {
            player.inventory.Draw(GetScreenWidth(), GetScreenHeight(), player.currentMode == GameMode::Creative);
        }
        else if (drawHud) {
            player.inventory.DrawHotbar(GetScreenWidth(), GetScreenHeight(), &BlockInfo::GetName);
            OverlayUI::DrawCrosshair(GetScreenWidth(), GetScreenHeight());
            if (player.digActive) {
                OverlayUI::DrawDigProgress(GetScreenWidth(), GetScreenHeight(),
                    player.digProgress, player.digBlocked);
            }

            if (player.currentMode == GameMode::Survival) {
                SurvivalHud sh;
                sh.health = player.health;
                sh.satiety = player.satiety;
                sh.hydration = player.hydration;
                sh.bodyTempC = player.bodyTempC;
                sh.airC = player.inventory.env.airC;
                OverlayUI::DrawSurvivalHud(GetScreenWidth(), GetScreenHeight(), sh);
            }

            if (!debugSystem.IsConsoleOpen() && player.targetedBlockID != 0) {
                OverlayUI::DrawWaila(GetScreenWidth(), GetScreenHeight(), player.targetedBlockID, BlockInfo::GetName(player.targetedBlockID));
            }
        }
        if (drawHud || questSystem->IsOpen())
            questSystem->Draw(*this, GetScreenWidth(), GetScreenHeight());
    }
    else if (currentState == GameState::MainMenu) {
        OverlayUI::DrawMainMenuOverlay(GetScreenHeight());
    }

    menuSystem->Render();
    if (drawHud || debugSystem.IsConsoleOpen()) debugSystem.Render2D();

    if (trailer && trailer->Active()) {
        trailer->DrawOverlay(GetScreenWidth(), GetScreenHeight());
        trailer->EndFrame();
    }
    if (autoShot && autoShot->Active()) autoShot->EndFrame(*this);

    EndDrawing();
}


void EidosEngine::RenderShadowPass() {
    if (shadowShader.id <= 0 || shadowMap.id <= 0) {
        shadowStrength = 0.0f;
        SetShaderValue(fogShader, shadowStrengthLoc, &shadowStrength, SHADER_UNIFORM_FLOAT);
        return;
    }

    Vector3 sunPos = skySystem.GetSunPosition();
    Vector3 sunDir = Vector3Normalize(sunPos);

    if (sunDir.y < -0.05f) {
        shadowStrength = 0.0f;
        SetShaderValue(fogShader, shadowStrengthLoc, &shadowStrength, SHADER_UNIFORM_FLOAT);
        return;
    }

    float shadowRange = (float)(renderDistance * Chunk::CHUNK_SIZE_X);
    float shadowDist = 300.0f;

    Matrix shadowProj = MatrixOrtho(-shadowRange, shadowRange, -shadowRange, shadowRange, 0.1f, shadowDist * 2.0f);

    Vector3 playerPos = player.camera.position;
    Vector3 shadowEye = Vector3Subtract(playerPos, Vector3Scale(sunDir, shadowDist));
    Vector3 shadowTarget = playerPos;
    Vector3 up = (fabsf(sunDir.y) > 0.999f) ? Vector3{1,0,0} : Vector3{0,1,0};

    Matrix shadowView = MatrixLookAt(shadowEye, shadowTarget, up);
    Matrix shadowMVP = MatrixMultiply(shadowView, shadowProj);

    Camera3D shadowCam = { 0 };
    shadowCam.position = shadowEye;
    shadowCam.target = shadowTarget;
    shadowCam.up = up;
    shadowCam.fovy = 90.0f;
    shadowCam.projection = CAMERA_ORTHOGRAPHIC;

    BeginTextureMode(shadowMap);
    ClearBackground(WHITE);

    BeginMode3D(shadowCam);
    rlEnableDepthTest();

    {
        std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
        int pChunkX = (int)floor(playerPos.x / Chunk::CHUNK_SIZE_X);
        int pChunkZ = (int)floor(playerPos.z / Chunk::CHUNK_SIZE_Z);

        for (auto& chunk : chunks) {
            if (abs(chunk->chunkX - pChunkX) > renderDistance || abs(chunk->chunkZ - pChunkZ) > renderDistance)
                continue;
            if (!chunk->hasMesh) continue;

            Model& m = chunk->GetModel();
            Shader savedShader = m.materials[0].shader;
            m.materials[0].shader = shadowShader;
            Vector3 pos = { (float)chunk->chunkX * 16, 0, (float)chunk->chunkZ * 16 };
            DrawModel(m, pos, 1.0f, WHITE);
            m.materials[0].shader = savedShader;
        }
    }

    EndMode3D();
    EndTextureMode();

    SetShaderValueMatrix(fogShader, shadowMVPLoc, shadowMVP);

    shadowStrength = Clamp(sunDir.y * 4.0f, 0.0f, 1.0f) * 0.85f;
    SetShaderValue(fogShader, shadowStrengthLoc, &shadowStrength, SHADER_UNIFORM_FLOAT);
    SetShaderValueTexture(fogShader, shadowMapTexLoc, shadowMap.depth);
}

bool EidosEngine::IsChunkInFrustum(Chunk* chunk) const {

    Vector3 chunkCenter = {
        (chunk->chunkX + 0.5f) * Chunk::CHUNK_SIZE_X,
        (float)std::max(chunk->maxY, 64) * 0.5f,
        (chunk->chunkZ + 0.5f) * Chunk::CHUNK_SIZE_Z
    };

    Vector3 fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
    Vector3 toChunk = Vector3Subtract(chunkCenter, player.camera.position);

    float dot = Vector3DotProduct(toChunk, fwd);
    return dot > -(Chunk::CHUNK_SIZE_X * 8);
}
