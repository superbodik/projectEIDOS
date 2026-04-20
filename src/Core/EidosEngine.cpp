#include "EidosEngine.h"
#include "CommandManager.h"
#include "../Inventory/BlockInfo.h"
#include "../World/Chunk.h"
#include <raymath.h>
#include <cmath>
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

EidosEngine::EidosEngine(int width, int height, std::string title)
    : screenWidth(width), screenHeight(height), worldGen(1337)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);
    Chunk::LoadAtlas();
    LoadConfig();

    player.camera.fovy = targetFOV;
    fogShader = LoadShader("assets/shaders/fog.vs", "assets/shaders/fog.fs");

    fogViewPosLoc = GetShaderLocation(fogShader, "viewPos");
    int fogColorLoc = GetShaderLocation(fogShader, "fogColor");
    int fogStartLoc = GetShaderLocation(fogShader, "fogStart");
    int fogEndLoc = GetShaderLocation(fogShader, "fogEnd");
    Vector4 fogColor = { 0.4f, 0.75f, 1.0f, 1.0f };
    float fogStart = (float)(renderDistance - 4) * Chunk::CHUNK_SIZE_X;
    float fogEnd = (float)(renderDistance)*Chunk::CHUNK_SIZE_X;
    SetShaderValue(fogShader, fogColorLoc, &fogColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(fogShader, fogStartLoc, &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader, fogEndLoc, &fogEnd, SHADER_UNIFORM_FLOAT);

    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    debugSystem.Initialize(this, &lua);
    menuSystem = std::make_unique<MenuSystem>(this);
    cmdManager = std::make_unique<CommandManager>(this);

    lua.set_function("locate", &EidosEngine::Locate, this);
    lua.set_function("save", [this]() { this->SaveWorld(false); });
    lua.set_function("exit", [this]() { this->appRunning = false; });
    lua.set_function("time", [this](std::string action, std::string value) {
        if (action == "set") {
            if (value == "day") this->skySystem.SetTime(8.0f);
            else if (value == "night") this->skySystem.SetTime(20.0f);
            else { try { this->skySystem.SetTime(std::stof(value)); } catch (...) {} }
        }
        });
    lua.set_function("gamemode", [this](int mode) { if (mode == 0)player.currentMode = GameMode::Survival; if (mode == 1)player.currentMode = GameMode::Creative; if (mode == 2)player.currentMode = GameMode::Spectator; });
    lua.set_function("gm", [this](int mode) { if (mode == 0)player.currentMode = GameMode::Survival; if (mode == 1)player.currentMode = GameMode::Creative; if (mode == 2)player.currentMode = GameMode::Spectator; });
    cmdManager->BindCommands(lua);

    appRunning = true;
    currentState = GameState::MainMenu;

    currentWorldName = "MenuPanorama";
    std::string path = "saves/" + currentWorldName;
    if (fs::exists(path + "/level.dat")) {
        std::ifstream in(path + "/level.dat"); int seed; float px, py, pz, tx, ty, tz;
        if (in >> seed >> px >> py >> pz >> tx >> ty >> tz) {
            worldGen.SetSeed(seed);
            player.position = { px, py, pz };
        }
        in.close();
    }
    else {
        worldGen.SetSeed(2026);
        int safeX = 0, safeZ = 0;
        int h = worldGen.GetHeight(safeX, safeZ);
        player.position = { (float)safeX + 0.5f, (float)h + 25.0f, (float)safeZ + 0.5f };
    }

    player.camera.position = player.position;
    player.camera.target = { player.position.x + 1.0f, player.position.y, player.position.z };

    unsigned int hwThreads = std::thread::hardware_concurrency();
    if (hwThreads == 0) hwThreads = 4;

    unsigned int terrainT = 1;
    unsigned int meshT = 1;

    if (hwThreads >= 6) {
        terrainT = 2;
        meshT = hwThreads - 3;
    }
    else if (hwThreads >= 4) {
        terrainT = 1;
        meshT = 2;
    }

    for (unsigned int i = 0; i < terrainT; i++) threadPool.emplace_back(&EidosEngine::TerrainWorker, this);
    for (unsigned int i = 0; i < meshT; i++) threadPool.emplace_back(&EidosEngine::MeshWorker, this);

    UpdateChunks();
}

EidosEngine::~EidosEngine() {
    if (currentState == GameState::Playing) SaveWorld(false);
    SaveConfig();
    appRunning = false;
    for (std::thread& t : threadPool) if (t.joinable()) t.join();
    UnloadShader(fogShader);
    UnloadWorld();
    CloseWindow();
}

void EidosEngine::LoadConfig() {
    std::string path = "config/settings.ini";
    if (!fs::exists("config")) fs::create_directories("config");
    if (fs::exists(path)) {
        std::ifstream file(path); std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line); std::string key; ss >> key;
            if (key == "RenderDistance") { int val; ss >> val; SetRenderDistance(val); }
            else if (key == "FOV") { float val; ss >> val; SetFOV(val); }
            else if (key == "MaxFPS") { int val; ss >> val; SetMaxFPS(val); }
            else if (key == "ShowFPS") { int val; ss >> val; showSimpleFPS = (bool)val; }
            else if (key == "Fullscreen") { int val; ss >> val; if (val == 1 && !IsWindowFullscreen()) ToggleFullscreen(); }
        }
    }
    else { SetRenderDistance(8); SetMaxFPS(165); SetFOV(70.0f); }
}

void EidosEngine::SaveConfig() {
    if (!fs::exists("config")) fs::create_directories("config");
    std::ofstream file("config/settings.ini");
    if (file.is_open()) {
        file << "RenderDistance " << renderDistance << "\n";
        file << "FOV " << targetFOV << "\n";
        file << "MaxFPS " << targetFPS << "\n";
        file << "ShowFPS " << (showSimpleFPS ? 1 : 0) << "\n";
        file << "Fullscreen " << (IsWindowFullscreen() ? 1 : 0) << "\n";
        file.close();
    }
}

void EidosEngine::CaptureScreenshot() {
    if (!fs::exists("screenshots")) fs::create_directories("screenshots");
    auto t = std::time(nullptr); auto tm = *std::localtime(&t);
    std::ostringstream oss; oss << "screenshots/" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".png";
    TakeScreenshot(oss.str().c_str()); debugSystem.Log("Screenshot saved.");
}

void EidosEngine::SetRenderDistance(int dist) {
    if (dist < 2) dist = 2; if (dist > 32) dist = 32;
    renderDistance = dist;
    float fogStart = (float)(renderDistance - 4) * Chunk::CHUNK_SIZE_X;
    float fogEnd = (float)(renderDistance)*Chunk::CHUNK_SIZE_X;
    if (fogShader.id > 0) {
        SetShaderValue(fogShader, GetShaderLocation(fogShader, "fogStart"), &fogStart, SHADER_UNIFORM_FLOAT);
        SetShaderValue(fogShader, GetShaderLocation(fogShader, "fogEnd"), &fogEnd, SHADER_UNIFORM_FLOAT);
    }
}

int EidosEngine::GetRenderDistance() const { return renderDistance; }
void EidosEngine::SetMaxFPS(int fps) { targetFPS = fps; SetTargetFPS(fps <= 0 ? 0 : fps); }
int EidosEngine::GetMaxFPS() const { return targetFPS; }
void EidosEngine::SetFOV(float fov) { targetFOV = std::clamp(fov, 30.0f, 110.0f); player.camera.fovy = targetFOV; }
float EidosEngine::GetFOV() const { return targetFOV; }
float EidosEngine::GetUIScale() const { return (float)GetScreenHeight() / 1080.0f; }
bool EidosEngine::ShouldClose() const { return WindowShouldClose() || !appRunning; }

void EidosEngine::ToggleFullscreen() {
    if (IsWindowFullscreen()) { ::ToggleFullscreen(); SetWindowSize(1280, 720); }
    else { int m = GetCurrentMonitor(); SetWindowSize(GetMonitorWidth(m), GetMonitorHeight(m)); ::ToggleFullscreen(); }
}

void EidosEngine::UnloadWorld() { std::lock_guard<std::recursive_mutex> lock(chunkListMutex); chunks.clear(); }

void EidosEngine::SaveWorld(bool autoSave) {
    if (currentState != GameState::Playing && currentState != GameState::Paused) return;
    std::string path = "saves/" + currentWorldName;
    if (!fs::exists(path)) fs::create_directories(path);

    if (!autoSave) TakeScreenshot((path + "/cover.png").c_str());

    std::ofstream out(path + "/level.dat");
    if (out.is_open()) {
        out << worldGen.GetSeed() << "\n" << player.position.x << " " << player.position.y << " " << player.position.z << "\n" << player.camera.target.x << " " << player.camera.target.y << " " << player.camera.target.z << "\n";
        out.close();
        if (!autoSave) debugSystem.Log("Saved level.dat");
    }
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    for (auto& c : chunks) { c->SaveToFile(path); }
}

void EidosEngine::LoadWorld(std::string worldName) {
    currentWorldName = worldName;
    std::string path = "saves/" + currentWorldName;

    { std::lock_guard<std::mutex> lock(terrainQueueMutex); std::queue<std::shared_ptr<Chunk>> empty; std::swap(terrainQueue, empty); }
    { std::lock_guard<std::mutex> lock(meshQueueMutex); std::queue<std::shared_ptr<Chunk>> empty; std::swap(meshQueue, empty); }

    UnloadWorld();
    if (fs::exists(path + "/level.dat")) {
        std::ifstream in(path + "/level.dat"); int seed; float px, py, pz, tx, ty, tz;
        if (in >> seed >> px >> py >> pz >> tx >> ty >> tz) {
            worldGen.SetSeed(seed);
            player.position = { px, py, pz };
            player.camera.position = player.position;
            player.camera.target = { px + 1.0f, py, pz };
            debugSystem.Log("Loaded world: " + worldName);
        } in.close();
    }
    else {
        int newSeed = GetRandomValue(0, 999999); if (currentState != GameState::CreateWorld) { worldGen.SetSeed(newSeed); }
        int safeX = 0, safeZ = 0; int h = worldGen.GetHeight(safeX, safeZ);
        player.position = { (float)safeX + 0.5f, (float)h + 2.0f, (float)safeZ + 0.5f }; player.velocity = { 0,0,0 };
        debugSystem.Log("Starting world: " + worldName);
    }
    for (int i = 0; i < 36; i++) player.inventory.slots[i] = { 0,0 };
    player.inventory.slots[0] = { 6, 64 }; player.inventory.slots[1] = { 17, 64 }; player.inventory.slots[2] = { 100, 64 };
    player.inventory.slots[3] = { 18, 64 }; player.inventory.slots[4] = { 3, 64 };
    currentState = GameState::Loading; UpdateChunks();
}

void EidosEngine::TerrainWorker() {
    while (appRunning) {
        std::shared_ptr<Chunk> task = nullptr;
        { std::lock_guard<std::mutex> lock(terrainQueueMutex); if (!terrainQueue.empty()) { task = terrainQueue.front(); terrainQueue.pop(); } }

        if (task) {
            if (task->state == 0) {
                std::string path = "saves/" + currentWorldName;
                if (!task->LoadFromFile(path)) task->GenerateTerrain(worldGen);
                task->isBusy = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void EidosEngine::MeshWorker() {
    while (appRunning) {
        std::shared_ptr<Chunk> task = nullptr;
        { std::lock_guard<std::mutex> lock(meshQueueMutex); if (!meshQueue.empty()) { task = meshQueue.front(); meshQueue.pop(); } }

        if (task) {
            if (task->state == 1 || task->dirty) {

                std::shared_ptr<Chunk> n[9];
                Chunk* raw_n[9] = { nullptr };
                bool neighborsReady = true;
                bool wasFirstBuild = (task->state == 1);

                {
                    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
                    int cx = task->chunkX; int cz = task->chunkZ;
                    for (auto& c : chunks) {
                        int dx = c->chunkX - cx;
                        int dz = c->chunkZ - cz;
                        if (dx >= -1 && dx <= 1 && dz >= -1 && dz <= 1) {
                            int idx = (dx + 1) + (dz + 1) * 3;
                            n[idx] = c;
                            raw_n[idx] = c.get();
                        }
                    }
                }

                for (int i = 0; i < 9; i++) {
                    if (i != 4 && n[i] && n[i]->state == 0) {
                        neighborsReady = false;
                        break;
                    }
                }

                if (neighborsReady) {
                    task->dirty = false;

                    bool lightChanged = task->BuildMeshCPU(worldGen, raw_n);
                    task->isBusy = false;

                    if (wasFirstBuild || lightChanged) {
                        std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
                        for (int i = 0; i < 9; i++) {
                            if (i != 4 && n[i] && n[i]->state >= 2) n[i]->dirty = true;
                        }
                    }

                }
                else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    std::lock_guard<std::mutex> qLock(meshQueueMutex);
                    meshQueue.push(task);
                    continue;
                }
            }
            else {
                task->isBusy = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void EidosEngine::UpdateChunks() {
    int pChunkX = (int)floor(player.position.x / Chunk::CHUNK_SIZE_X);
    int pChunkZ = (int)floor(player.position.z / Chunk::CHUNK_SIZE_Z);
    int unloadDist = renderDistance + 2;
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);

    for (auto it = chunks.begin(); it != chunks.end();) {
        Chunk* c = it->get();
        if ((abs(c->chunkX - pChunkX) > unloadDist || abs(c->chunkZ - pChunkZ) > unloadDist) && !c->isBusy) {
            c->SaveToFile("saves/" + currentWorldName); it = chunks.erase(it);
        }
        else ++it;
    }

    int uploads = 0;
    for (auto& c : chunks) { if (c->state == 2 && uploads < 20) { c->UploadMeshGPU(); uploads++; } }

    for (auto& c : chunks) {
        if (!c->isBusy && (c->dirty || c->state == 1)) {
            c->isBusy = true;
            std::lock_guard<std::mutex> qLock(meshQueueMutex);
            meshQueue.push(c);
        }
    }

    size_t tqSize = 0; { std::lock_guard<std::mutex> qLock(terrainQueueMutex); tqSize = terrainQueue.size(); }
    if (tqSize > 100) return;

    int scheduled = 0;
    int genDist = renderDistance + 1;

    for (int x = -genDist; x <= genDist; x++) {
        for (int z = -genDist; z <= genDist; z++) {
            int targetX = pChunkX + x; int targetZ = pChunkZ + z;

            bool exists = false;
            for (auto& c : chunks) {
                if (c->chunkX == targetX && c->chunkZ == targetZ) {
                    exists = true;
                    if (c->state == 0 && !c->isBusy) {
                        c->isBusy = true;
                        std::lock_guard<std::mutex> qLock(terrainQueueMutex);
                        terrainQueue.push(c);
                    }
                    break;
                }
            }
            if (!exists) {
                if (scheduled >= 2) continue;
                auto newChunk = std::make_shared<Chunk>(targetX, targetZ);
                newChunk->isBusy = true; chunks.push_back(newChunk);
                std::lock_guard<std::mutex> qLock(terrainQueueMutex);
                terrainQueue.push(newChunk);
                scheduled++;
            }
        }
    }
}

void EidosEngine::Update() {
    if (IsKeyPressed(KEY_F2)) CaptureScreenshot();
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if (currentState != GameState::MainMenu) debugSystem.Update();

    if (currentState == GameState::Playing || currentState == GameState::Loading) {
        skySystem.Update(GetFrameTime(), player.position);
    }
    menuSystem->Update();

    bool showCursor = (currentState != GameState::Playing && currentState != GameState::Loading) || debugSystem.IsConsoleOpen() || player.inventory.isOpen;
    if (showCursor) { if (IsCursorHidden()) EnableCursor(); }
    else { if (!IsCursorHidden()) DisableCursor(); }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (debugSystem.IsConsoleOpen()) debugSystem.ToggleConsole();
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

        if (!debugSystem.IsConsoleOpen()) if (IsKeyPressed(KEY_E)) player.inventory.Toggle();

        if (!debugSystem.IsConsoleOpen() && !player.inventory.isOpen) {
            float dt = GetFrameTime();
            player.Update([this](int x, int y, int z) { return (BlockType)this->GetBlockAt(x, y, z); }, dt);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) {
                    int targetX = (int)floor(hit.x);
                    int targetY = (int)floor(hit.y);
                    int targetZ = (int)floor(hit.z);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        SetBlockGlobal(targetX, targetY, targetZ, 0);
                    }
                    else {
                        int pX = (int)floor(player.position.x); int pY = (int)floor(player.position.y); int pZ = (int)floor(player.position.z);
                        if ((hit.px != pX || hit.pz != pZ) || (hit.py != pY && hit.py != pY + 1)) {
                            int id = player.inventory.GetSelectedBlockID();
                            if (id != 0) {
                                targetX = hit.px; targetY = hit.py; targetZ = hit.pz;
                                SetBlockGlobal(targetX, targetY, targetZ, id);
                                if (player.currentMode != GameMode::Creative) player.inventory.ConsumeSelectedItem();
                            }
                        }
                    }

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
                }
            }
        }
        else {
            if (player.inventory.isOpen) player.inventory.UpdateInput();
            player.Update([this](int x, int y, int z) { return (BlockType)this->GetBlockAt(x, y, z); }, 0.0f);
        }
        UpdateChunks();
    }
    else if (currentState == GameState::Loading) {
        UpdateChunks();
        Chunk* spawnChunk = GetChunkAt((int)player.position.x, (int)player.position.z);

        int waitRadius = std::min(12, renderDistance);

        if (spawnChunk != nullptr && spawnChunk->state >= 3 && IsAreaLoaded(waitRadius)) {
            int h = worldGen.GetHeight((int)player.position.x, (int)player.position.z);
            float safeY = (float)std::max(h, 64) + 2.0f;
            if (player.position.y < safeY) player.position.y = safeY;
            player.velocity = { 0,0,0 };
            currentState = GameState::Playing;
        }
    }
    else {
        float time = (float)GetTime() * 0.05f;
        player.camera.position = { player.position.x + sinf(time) * 50.0f, player.position.y + 15.0f, player.position.z + cosf(time) * 50.0f };
        player.camera.target = player.position;
        UpdateChunks();
    }
    float camPos[3] = { player.camera.position.x, player.camera.position.y, player.camera.position.z };
    SetShaderValue(fogShader, fogViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
}

void EidosEngine::Render() {
    BeginDrawing();
    ClearBackground(skySystem.GetSkyColor());

    if (currentState == GameState::Loading) {
        OverlayUI::DrawLoadingScreen(GetScreenWidth(), GetScreenHeight());
        EndDrawing();
        return;
    }

    BeginMode3D(player.camera);
    skySystem.Draw();

    if (fogShader.id > 0) {
        Color c = skySystem.GetFogColor(); float vec[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f };
        SetShaderValue(fogShader, GetShaderLocation(fogShader, "fogColor"), vec, SHADER_UNIFORM_VEC4);
    }

    {
        std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
        for (auto& chunk : chunks) {
            int pChunkX = (int)floor(player.position.x / Chunk::CHUNK_SIZE_X);
            int pChunkZ = (int)floor(player.position.z / Chunk::CHUNK_SIZE_Z);
            if (abs(chunk->chunkX - pChunkX) <= renderDistance && abs(chunk->chunkZ - pChunkZ) <= renderDistance) {
                if (chunk->hasMesh && IsChunkInFrustum(chunk.get())) {
                    chunk->Draw();
                    if (debugSystem.showChunkBorders) {
                        Vector3 pos = { (float)chunk->chunkX * 16, 0, (float)chunk->chunkZ * 16 };
                        DrawCubeWiresV({ pos.x + 8, 128, pos.z + 8 }, { 16, 256, 16 }, YELLOW);
                    }
                }
            }
        }
    }

    if (currentState == GameState::Playing && !debugSystem.IsConsoleOpen() && !player.inventory.isOpen) {
        RayHitInfo hit = CastRay(5.0f);
        if (hit.hit) DrawCubeWires({ hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f }, 1.01f, 1.01f, 1.01f, BLACK);
    }
    EndMode3D();

    if (showSimpleFPS) DrawText(TextFormat("%d FPS", GetFPS()), 10, 10, 20, GREEN);

    if (currentState == GameState::Playing) {
        if (player.inventory.isOpen) {
            player.inventory.Draw(GetScreenWidth(), GetScreenHeight());
        }
        else {
            player.inventory.DrawHotbar(GetScreenWidth(), GetScreenHeight(), &BlockInfo::GetName);
            OverlayUI::DrawCrosshair(GetScreenWidth(), GetScreenHeight());

            if (!debugSystem.IsConsoleOpen() && player.targetedBlockID != 0) {
                OverlayUI::DrawWaila(GetScreenWidth(), GetScreenHeight(), player.targetedBlockID, BlockInfo::GetName(player.targetedBlockID));
            }
        }
    }
    else if (currentState == GameState::MainMenu) {
        OverlayUI::DrawMainMenuOverlay(GetScreenHeight());
    }

    menuSystem->Render();
    debugSystem.Render2D();
    EndDrawing();
}

std::string EidosEngine::GetBiomeName(int x, int y, int z) { (void)y; return worldGen.GetBiomeName(x, z); }
std::string EidosEngine::GetBlockName(int type) const { return BlockInfo::GetName(type); }

void EidosEngine::SetBlockGlobal(int x, int y, int z, int type) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) c->SetBlock((x % 16 + 16) % 16, y, (z % 16 + 16) % 16, type);
}
void EidosEngine::SetBlockGlobalFast(int x, int y, int z, int type) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) c->SetBlockRaw((x % 16 + 16) % 16, y, (z % 16 + 16) % 16, type);
}
Chunk* EidosEngine::GetChunkAt(int x, int z) {
    int cx = (int)floor((float)x / 16.0f); int cz = (int)floor((float)z / 16.0f);
    for (auto& c : chunks) { if (c->chunkX == cx && c->chunkZ == cz) return c.get(); }
    return nullptr;
}
BlockType EidosEngine::GetBlockAt(int x, int y, int z) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) return c->GetBlock((x % 16 + 16) % 16, y, (z % 16 + 16) % 16);
    return BlockType::Air;
}
RayHitInfo EidosEngine::CastRay(float maxDistance) {
    RayHitInfo info = { false, 0,0,0, 0,0,0 };
    Vector3 origin = player.camera.position;
    Vector3 dir = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
    float step = 0.05f;
    int lx = (int)floor(origin.x); int ly = (int)floor(origin.y); int lz = (int)floor(origin.z);
    for (float d = 0; d < maxDistance; d += step) {
        Vector3 p = Vector3Add(origin, Vector3Scale(dir, d));
        int bx = (int)floor(p.x); int by = (int)floor(p.y); int bz = (int)floor(p.z);
        if (GetBlockAt(bx, by, bz) != BlockType::Air) {
            info.hit = true; info.x = bx; info.y = by; info.z = bz; info.px = lx; info.py = ly; info.pz = lz; return info;
        }
        lx = bx; ly = by; lz = bz;
    }
    return info;
}
bool EidosEngine::IsChunkInFrustum(Chunk* chunk) const { (void)chunk; return true; }
bool EidosEngine::IsAreaLoaded(int radius) {
    int cx = (int)floor(player.position.x / Chunk::CHUNK_SIZE_X);
    int cz = (int)floor(player.position.z / Chunk::CHUNK_SIZE_Z);
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            Chunk* c = GetChunkAt((cx + x) * 16, (cz + z) * 16);
            if (!c || c->state < 3) return false;
        }
    }
    return true;
}
void EidosEngine::Locate(std::string type, std::string value) {
    if (type == "biome") {
        debugSystem.Log("Searching for biome: " + value + "...");
        BiomeType target = worldGen.GetBiomeFromString(value);
        auto result = worldGen.FindBiome((int)player.position.x, (int)player.position.z, target, 5000, 64);
        if (result.found) {
            int y = worldGen.GetHeight(result.x, result.z) + 5;
            debugSystem.Log("Found " + value + " at " + std::to_string(result.x) + ", " + std::to_string(result.z));
            player.position = { (float)result.x, (float)y, (float)result.z }; LoadWorld(currentWorldName);
        }
        else { debugSystem.Log("Could not find biome " + value + " nearby."); }
    }
    else { debugSystem.Log("Unknown locate type. Usage: locate biome <Name>"); }
}
std::string EidosEngine::GetDirectionString(float rotationY) const {
    if (rotationY >= 315 || rotationY < 45) return "North"; if (rotationY >= 45 && rotationY < 135) return "East";
    if (rotationY >= 135 && rotationY < 225) return "South"; return "West";
}
Player& EidosEngine::GetPlayer() { return player; }
size_t EidosEngine::GetQueueSize() {
    size_t s = 0;
    { std::lock_guard<std::mutex> lk(terrainQueueMutex); s += terrainQueue.size(); }
    { std::lock_guard<std::mutex> lk(meshQueueMutex); s += meshQueue.size(); }
    return s;
}
void EidosEngine::ToggleDebug() { debugSystem.ToggleOverlay(); }
void EidosEngine::CloseApp() { appRunning = false; }