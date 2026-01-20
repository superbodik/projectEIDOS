#include "EidosEngine.h"
#include "CommandManager.h"
#include <cmath>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

#ifdef _WIN32
#define NOGDI              
#define NOUSER             
#include <windows.h>
#include <psapi.h>
#if defined(DrawText)
#undef DrawText
#endif
#pragma comment(lib, "psapi.lib")
#endif

std::string GetNameForInventory(int id) {
    switch (id) {
    case 1: return "Water";
    case 2: return "Water Source";
    case 3: return "Lava";
    case 4: return "Lava Source";
    case 5: return "Bedrock";
    case 6: return "Grass";
    case 7: return "Dirt";
    case 8: return "Coarse Dirt";
    case 9: return "Mud";
    case 10: return "Clay";
    case 11: return "Sand";
    case 12: return "Red Sand";
    case 13: return "Gravel";
    case 14: return "Silt";
    case 15: return "Peat";
    case 20: return "Limestone";
    case 21: return "Chalk";
    case 22: return "Shale";
    case 23: return "Claystone";
    case 24: return "Sandstone";
    case 25: return "Red Sandstone";
    case 26: return "Conglomerate";
    case 27: return "Dolomite";
    case 28: return "Chert";
    case 30: return "Granite";
    case 31: return "Diorite";
    case 32: return "Gabbro";
    case 35: return "Rhyolite";
    case 36: return "Basalt";
    case 37: return "Andesite";
    case 38: return "Dacite";
    case 40: return "Quartzite";
    case 41: return "Slate";
    case 42: return "Phyllite";
    case 43: return "Schist";
    case 44: return "Gneiss";
    case 45: return "Marble";
    case 50: return "Native Copper";
    case 51: return "Malachite";
    case 52: return "Tetrahedrite";
    case 53: return "Hematite";
    case 54: return "Magnetite";
    case 55: return "Limonite";
    case 56: return "Bituminous Coal";
    case 57: return "Lignite";
    case 60: return "Native Gold";
    case 61: return "Native Silver";
    case 62: return "Cassiterite";
    case 63: return "Sphalerite";
    case 64: return "Bismuthinite";
    case 65: return "Galena";
    case 66: return "Kimberlite";
    case 100: return "Oak Log";
    case 101: return "Oak Leaves";
    case 102: return "Spruce Log";
    case 103: return "Spruce Leaves";
    case 104: return "Birch Log";
    case 105: return "Birch Leaves";
    case 106: return "Acacia Log";
    case 107: return "Acacia Leaves";
    case 108: return "Jungle Log";
    case 109: return "Jungle Leaves";
    case 110: return "Cactus";
    case 111: return "Tall Grass";
    case 112: return "Dead Bush";
    case 113: return "Rose";
    case 114: return "Dandelion";
    case 115: return "Brown Mushroom";
    case 116: return "Red Mushroom";
    case 117: return "Sugar Cane";
    case 118: return "Pumpkin";
    case 119: return "Melon";
    case 120: return "Snow";
    case 121: return "Ice";
    case 122: return "Packed Ice";
    default: return "Block #" + std::to_string(id);
    }
}

EidosEngine::EidosEngine(int width, int height, std::string title)
    : screenWidth(width), screenHeight(height), worldGen(1337)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, title.c_str());
    SetExitKey(KEY_NULL);

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
    cmdManager->BindCommands(lua);

    player.position = { 0.5f, 150.0f, 0.5f };
    player.camera.position = player.position;

    appRunning = true;
    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) threadCount = 4;
    if (threadCount > 2) threadCount -= 2;
    else if (threadCount > 1) threadCount = 1;

    for (unsigned int i = 0; i < threadCount; i++) threadPool.emplace_back(&EidosEngine::GeneratorThreadWorker, this);
    UpdateChunks();
}

EidosEngine::~EidosEngine() {
    if (currentState == GameState::Playing) SaveWorld();
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
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string key;
            ss >> key;
            if (key == "RenderDistance") { int val; ss >> val; SetRenderDistance(val); }
            else if (key == "FOV") { float val; ss >> val; SetFOV(val); }
            else if (key == "MaxFPS") { int val; ss >> val; SetMaxFPS(val); }
            else if (key == "ShowFPS") { int val; ss >> val; showSimpleFPS = (bool)val; }
            else if (key == "Fullscreen") { int val; ss >> val; if (val == 1 && !IsWindowFullscreen()) ToggleFullscreen(); }
        }
    }
    else {
        SetRenderDistance(8);
        SetMaxFPS(165);
        SetFOV(70.0f);
    }
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
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << "screenshots/" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".png";
    std::string filename = oss.str();
    TakeScreenshot(filename.c_str());
    debugSystem.Log("Screenshot saved: " + filename);
}

void EidosEngine::SetFOV(float fov) {
    targetFOV = std::clamp(fov, 30.0f, 110.0f);
    player.camera.fovy = targetFOV;
}

float EidosEngine::GetUIScale() const {
    return (float)GetScreenHeight() / 1080.0f;
}

bool EidosEngine::ShouldClose() const { return WindowShouldClose() || !appRunning; }

void EidosEngine::ToggleFullscreen() {
    if (IsWindowFullscreen()) {
        ::ToggleFullscreen();
        SetWindowSize(1280, 720);
    }
    else {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ::ToggleFullscreen();
    }
}

void EidosEngine::UnloadWorld() {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    chunks.clear();
}

void EidosEngine::SaveWorld() {
    if (currentState != GameState::Playing && currentState != GameState::Paused) return;
    std::string path = "saves/" + currentWorldName;
    if (!fs::exists(path)) fs::create_directories(path);
    TakeScreenshot((path + "/cover.png").c_str());
    std::ofstream out(path + "/level.dat");
    if (out.is_open()) {
        out << worldGen.GetSeed() << "\n";
        out << player.position.x << " " << player.position.y << " " << player.position.z << "\n";
        out << player.camera.target.x << " " << player.camera.target.y << " " << player.camera.target.z << "\n";
        out.close();
        debugSystem.Log("Saved level.dat");
    }
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    int count = 0;
    for (auto& c : chunks) { if (c->SaveToFile(path)) count++; }
    if (count > 0) debugSystem.Log("Saved " + std::to_string(count) + " modified chunks.");
}

void EidosEngine::LoadWorld(std::string worldName) {
    currentWorldName = worldName;
    std::string path = "saves/" + currentWorldName;

    // Очистка очередей
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::queue<std::shared_ptr<Chunk>> empty;
        std::swap(generationQueue, empty);
    }

    UnloadWorld();

    // Попытка загрузить существующий мир
    if (fs::exists(path + "/level.dat")) {
        std::ifstream in(path + "/level.dat");
        int seed; float px, py, pz, tx, ty, tz;
        if (in >> seed >> px >> py >> pz >> tx >> ty >> tz) {
            worldGen.SetSeed(seed);
            player.position = { px, py, pz };
            player.camera.position = player.position;
            player.camera.target = { tx, ty, tz };
            debugSystem.Log("Loaded world: " + worldName);
        }
        in.close();
    }
    // Создание НОВОГО мира
    else {
        int newSeed = GetRandomValue(0, 999999);
        if (currentState != GameState::CreateWorld) { worldGen.SetSeed(newSeed); }

        debugSystem.Log("Generating new spawn point...");

        int safeX = 0;
        int safeZ = 0;
        bool foundLand = false;

        // Попытка 1: Ищем сушу (высота > 65)
        for (int i = 0; i < 200; i++) {
            // Разбрасываем проверки широко, чтобы вылезти из океана
            int testX = (GetRandomValue(0, 100) - 50) * 16;
            int testZ = (GetRandomValue(0, 100) - 50) * 16;

            int h = worldGen.GetHeight(testX, testZ);

            // Если нашли высоту выше уровня моря (64)
            if (h > 65) {
                safeX = testX;
                safeZ = testZ;
                foundLand = true;
                debugSystem.Log("Land found at attempt " + std::to_string(i));
                break;
            }
        }

        // Финальный расчет высоты
        int terrainHeight = worldGen.GetHeight(safeX, safeZ);
        float spawnY;

        if (foundLand) {
            // Если это суша - ставим на блок
            spawnY = (float)terrainHeight + 2.0f;
        }
        else {
            // Если кругом вода - ставим НА ПОВЕРХНОСТЬ ВОДЫ (обычно 64)
            // Если дно выше 64, ставим на дно, иначе плаваем
            int waterLevel = 64;
            spawnY = (float)(std::max(terrainHeight, waterLevel)) + 2.0f;
            debugSystem.Log("Spawned in ocean (surfaced).");
        }

        player.position = { (float)safeX + 0.5f, spawnY, (float)safeZ + 0.5f };
        player.velocity = { 0,0,0 };

        debugSystem.Log("Starting world: " + worldName + " Seed: " + std::to_string(worldGen.GetSeed()));
    }

    // Стартовый инвентарь
    for (int i = 0; i < 36; i++) player.inventory.slots[i] = { 0,0 };
    player.inventory.slots[0] = { 6, 64 };  // Grass
    player.inventory.slots[1] = { 7, 64 };  // Dirt
    player.inventory.slots[2] = { 11, 64 }; // Sand
    player.inventory.slots[3] = { 100, 64 }; // Logs
    player.inventory.slots[4] = { 5, 64 };   // Bedrock

    currentState = GameState::Loading;
    UpdateChunks();
}
void EidosEngine::GeneratorThreadWorker() {
    while (appRunning) {
        std::shared_ptr<Chunk> task = nullptr;
        { std::lock_guard<std::mutex> lock(queueMutex); if (!generationQueue.empty()) { task = generationQueue.front(); generationQueue.pop(); } }
        if (task) {
            if (task->state == 0) {
                std::string path = "saves/" + currentWorldName;
                if (!task->LoadFromFile(path)) task->GenerateTerrain(worldGen);
            }
            if (task->state == 1 || task->shouldRender) task->BuildMeshCPU(worldGen);
            task->isBusy = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
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

void EidosEngine::SetMaxFPS(int fps) {
    targetFPS = fps;
    SetTargetFPS(fps <= 0 ? 0 : fps);
}

void EidosEngine::UpdateChunks() {
    int pChunkX = (int)floor(player.position.x / Chunk::CHUNK_SIZE_X);
    int pChunkZ = (int)floor(player.position.z / Chunk::CHUNK_SIZE_Z);
    int unloadDist = renderDistance + 2;
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    for (auto it = chunks.begin(); it != chunks.end();) {
        Chunk* c = it->get();
        if ((abs(c->chunkX - pChunkX) > unloadDist || abs(c->chunkZ - pChunkZ) > unloadDist) && !c->isBusy) {
            c->SaveToFile("saves/" + currentWorldName);
            it = chunks.erase(it);
        }
        else ++it;
    }
    int uploads = 0;
    for (auto& c : chunks) {
        if (c->state == 2 && uploads < 2) { c->UploadMeshGPU(); uploads++; }
    }
    size_t qSize = 0; { std::lock_guard<std::mutex> qLock(queueMutex); qSize = generationQueue.size(); }
    if (qSize > 50) return;
    int scheduled = 0;
    for (int x = -renderDistance; x <= renderDistance; x++) {
        for (int z = -renderDistance; z <= renderDistance; z++) {
            int targetX = pChunkX + x; int targetZ = pChunkZ + z;
            if (abs(x) + abs(z) > renderDistance) continue;
            bool exists = false;
            for (auto& c : chunks) { if (c->chunkX == targetX && c->chunkZ == targetZ) { exists = true; if (c->shouldRender && !c->isBusy) { c->isBusy = true; { std::lock_guard<std::mutex> qLock(queueMutex); generationQueue.push(c); } } break; } }
            if (!exists) {
                if (scheduled >= 4) continue;
                auto newChunk = std::make_shared<Chunk>(targetX, targetZ);
                newChunk->isBusy = true;
                chunks.push_back(newChunk);
                { std::lock_guard<std::mutex> qLock(queueMutex); generationQueue.push(newChunk); }
                scheduled++;
            }
        }
    }
}

void EidosEngine::DrawLoadingScreen() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, BLACK);
    const char* text = "GENERATING TERRAIN...";
    int w = MeasureText(text, 30);
    DrawText(text, (sw - w) / 2, sh / 2 - 15, 30, WHITE);
    DrawRectangleLines((sw - 400) / 2, sh / 2 + 40, 400, 20, DARKGRAY);
    static float timer = 0.0f; timer += GetFrameTime();
    int progress = (int)(sinf(timer * 2.0f) * 100.0f + 100.0f) + 200;
    DrawRectangle((sw - 400) / 2, sh / 2 + 40, progress % 400, 20, WHITE);
}

void EidosEngine::Update() {
    if (IsKeyPressed(KEY_F2)) CaptureScreenshot();
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    if (currentState != GameState::MainMenu) debugSystem.Update();

    if (IsKeyDown(KEY_F3) && IsKeyPressed(KEY_G)) showChunkBorders = !showChunkBorders;

    menuSystem->Update();

    bool showCursor = false;
    if (currentState != GameState::Playing && currentState != GameState::Loading) showCursor = true;
    if (debugSystem.IsConsoleOpen() || player.inventory.isOpen) showCursor = true;

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
        if (!debugSystem.IsConsoleOpen()) {
            if (IsKeyPressed(KEY_E)) player.inventory.Toggle();
        }

        if (!debugSystem.IsConsoleOpen() && !player.inventory.isOpen) {
            float dt = GetFrameTime();
            player.Update([this](int x, int y, int z) { return this->GetBlockAt(x, y, z); }, dt);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) SetBlockGlobal(hit.x, hit.y, hit.z, 0);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) {
                    int pX = (int)floor(player.position.x); int pY = (int)floor(player.position.y); int pZ = (int)floor(player.position.z);
                    if ((hit.px != pX || hit.pz != pZ) || (hit.py != pY && hit.py != pY + 1)) {
                        int id = player.inventory.GetSelectedBlockID();
                        if (id != 0) {
                            SetBlockGlobal(hit.px, hit.py, hit.pz, id);
                            if (player.currentMode != GameMode::Creative) player.inventory.ConsumeSelectedItem();
                        }
                    }
                }
            }
        }
        else {
            if (player.inventory.isOpen) player.inventory.UpdateInput();
            player.Update([this](int x, int y, int z) { return this->GetBlockAt(x, y, z); }, 0.0f);
        }
        UpdateChunks();
    }
    else if (currentState == GameState::Loading) {
        UpdateChunks();

        Chunk* spawnChunk = GetChunkAt((int)player.position.x, (int)player.position.z);

        if (spawnChunk != nullptr && spawnChunk->state >= 3 && IsAreaLoaded(1)) {

            int h = worldGen.GetHeight((int)player.position.x, (int)player.position.z);
            int waterLevel = 64;
            float safeY = (float)std::max(h, waterLevel) + 2.0f;

            if (player.position.y < safeY) {
                player.position.y = safeY;
            }

            player.velocity = { 0,0,0 };
            currentState = GameState::Playing;
        }
    }
    else if (currentState != GameState::Playing && currentState != GameState::Loading) {
        float time = (float)GetTime() * 0.1f;
        player.camera.position.x = player.position.x + sinf(time) * 40.0f;
        player.camera.position.z = player.position.z + cosf(time) * 40.0f;
        player.camera.position.y = player.position.y + 20.0f;
        player.camera.target = player.position;

        int pChunkX = (int)floor(player.position.x / Chunk::CHUNK_SIZE_X);
        int pChunkZ = (int)floor(player.position.z / Chunk::CHUNK_SIZE_Z);
        {
            std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
            size_t qSize = 0; { std::lock_guard<std::mutex> lk(queueMutex); qSize = generationQueue.size(); }
            if (qSize <= 50) {
                int scheduled = 0;
                for (int x = -4; x <= 4; x++) {
                    for (int z = -4; z <= 4; z++) {
                        int tx = pChunkX + x; int tz = pChunkZ + z;
                        bool exists = false;
                        for (auto& c : chunks) if (c->chunkX == tx && c->chunkZ == tz) { exists = true; break; }
                        if (!exists) {
                            if (scheduled > 2) continue;
                            auto nc = std::make_shared<Chunk>(tx, tz);
                            nc->isBusy = true;
                            { std::lock_guard<std::mutex> lk(queueMutex); generationQueue.push(nc); }
                            chunks.push_back(nc);
                            scheduled++;
                        }
                    }
                }
            }
            int ups = 0;
            for (auto& c : chunks) if (c->state == 2 && ups < 2) { c->UploadMeshGPU(); ups++; }
        }
    }
    float camPos[3] = { player.camera.position.x, player.camera.position.y, player.camera.position.z };
    SetShaderValue(fogShader, fogViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
}

void EidosEngine::Render() {
    BeginDrawing();
    ClearBackground(SKYBLUE);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    if (currentState == GameState::Loading) {
        DrawLoadingScreen();
        EndDrawing();
        return;
    }
    BeginMode3D(player.camera);
    {
        std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
        for (auto& chunk : chunks) {
            if (chunk->state == 3 && IsChunkInFrustum(chunk.get())) {
                chunk->Draw();
                if (debugSystem.showChunkBorders) {
                    Vector3 pos = { (float)chunk->chunkX * 16, 0, (float)chunk->chunkZ * 16 };
                    DrawCubeWiresV({ pos.x + 8, 128, pos.z + 8 }, { 16, 256, 16 }, YELLOW);
                }
            }
        }
    }
    if (currentState == GameState::Playing && !debugSystem.IsConsoleOpen() && !player.inventory.isOpen) {
        RayHitInfo hit = CastRay(5.0f);
        if (hit.hit) DrawCubeWires({ hit.x + 0.5f, hit.y + 0.5f, hit.z + 0.5f }, 1.01f, 1.01f, 1.01f, BLACK);
    }
    EndMode3D();

    float scale = GetUIScale();

    if (showSimpleFPS) {
        DrawText(TextFormat("%d FPS", GetFPS()), (int)(10 * scale), (int)(10 * scale), (int)(20 * scale), GREEN);
    }

    if (currentState == GameState::Playing) {
        if (player.inventory.isOpen) {
            player.inventory.Draw(sw, sh);
        }
        else {
            player.inventory.DrawHotbar(sw, sh, &GetNameForInventory);
            DrawText("+", sw / 2 - (int)(5 * scale), sh / 2 - (int)(10 * scale), (int)(20 * scale), Fade(WHITE, 0.8f));
            if (!debugSystem.IsConsoleOpen() && player.targetedBlockID != 0) {
                std::string blockName = GetNameForInventory(player.targetedBlockID);
                int w = MeasureText(blockName.c_str(), (int)(20 * scale));
                DrawRectangle(sw / 2 - w / 2 - (int)(5 * scale), (int)(55 * scale), w + (int)(10 * scale), (int)(25 * scale), Fade(BLACK, 0.5f));
                DrawText(blockName.c_str(), sw / 2 - w / 2, (int)(58 * scale), (int)(20 * scale), WHITE);
            }
        }
    }

    menuSystem->Render();
    debugSystem.Render2D();
    EndDrawing();
}

std::string EidosEngine::GetBiomeName(int x, int y, int z) {
    (void)y;
    float temp = worldGen.GetTemperature(x, z);
    if (temp < 0.25f) return "Tundra";
    if (temp > 0.75f) return "Jungle/Desert";
    return "Forest/Plains";
}

std::string EidosEngine::GetBlockName(int type) const {
    return GetNameForInventory(type);
}

void EidosEngine::SetBlockGlobal(int x, int y, int z, int type) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) c->SetBlock((x % 16 + 16) % 16, y, (z % 16 + 16) % 16, type);
}

void EidosEngine::SetBlockGlobalFast(int x, int y, int z, int type) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) c->SetBlockRaw((x % 16 + 16) % 16, y, (z % 16 + 16) % 16, type);
}

Chunk* EidosEngine::GetChunkAt(int x, int z) {
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    int cx = (int)floor((float)x / 16); int cz = (int)floor((float)z / 16);
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