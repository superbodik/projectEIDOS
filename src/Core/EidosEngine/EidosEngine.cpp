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

EidosEngine::EidosEngine(int width, int height, std::string title)
    : screenWidth(width), screenHeight(height), worldGen(1337)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title.c_str());
    SetWindowMinSize(854, 480);
    SetExitKey(KEY_NULL);
    Chunk::LoadAtlas();
    Chunk::LoadWaterAtlas();
    LoadConfig();

    player.camera.fovy = targetFOV;
    fogShader = LoadShader("assets/shaders/fog.vs", "assets/shaders/fog.fs");
    Chunk::fogShader = fogShader;
    debugSystem.Log("Fog shader ID: " + std::to_string(fogShader.id));
    if (fogShader.id <= 0) {
        TraceLog(LOG_ERROR, "FOG SHADER FAILED TO LOAD! Check assets/shaders/fog.vs and fog.fs");
    }

    fogViewPosLoc = GetShaderLocation(fogShader, "viewPos");
    fogColorLoc = GetShaderLocation(fogShader, "fogColor");
    fogWindVecLoc = GetShaderLocation(fogShader, "windVec");
    fogWindTimeLoc = GetShaderLocation(fogShader, "windTime");
    fogStartLoc = GetShaderLocation(fogShader, "fogStart");
    fogEndLoc = GetShaderLocation(fogShader, "fogEnd");
    fogSunDirLoc = GetShaderLocation(fogShader, "sunDir");
    Vector4 fogColor = { 0.4f, 0.75f, 1.0f, 1.0f };
    float fogStart = (float)(renderDistance - 4) * Chunk::CHUNK_SIZE_X;
    float fogEnd = (float)(renderDistance)*Chunk::CHUNK_SIZE_X;
    SetShaderValue(fogShader, fogColorLoc, &fogColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(fogShader, fogStartLoc, &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader, fogEndLoc, &fogEnd, SHADER_UNIFORM_FLOAT);

    waterShader = LoadShader("assets/shaders/water.vs", "assets/shaders/water.fs");
    Chunk::waterShader = waterShader;
    if (waterShader.id <= 0) {
        TraceLog(LOG_ERROR, "WATER SHADER FAILED TO LOAD! Check assets/shaders/water.vs and water.fs");
    }
    waterViewPosLoc = GetShaderLocation(waterShader, "viewPos");
    waterFogColorLoc = GetShaderLocation(waterShader, "fogColor");
    waterFogStartLoc = GetShaderLocation(waterShader, "fogStart");
    waterFogEndLoc = GetShaderLocation(waterShader, "fogEnd");
    waterTimeLoc = GetShaderLocation(waterShader, "waterTime");
    waterSunDirLoc = GetShaderLocation(waterShader, "sunDir");
    waterSkyTintLoc = GetShaderLocation(waterShader, "skyTint");
    waterUnderwaterLoc = GetShaderLocation(waterShader, "underwater");
    waterWindLoc = GetShaderLocation(waterShader, "windStrength");
    float waterFrames = (float)Chunk::WATER_FRAMES;
    SetShaderValue(waterShader, GetShaderLocation(waterShader, "waterFrames"), &waterFrames, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterFogColorLoc, &fogColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(waterShader, waterFogStartLoc, &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(waterShader, waterFogEndLoc, &fogEnd, SHADER_UNIFORM_FLOAT);

    shadowShader = LoadShader("assets/shaders/shadow.vs", "assets/shaders/shadow.fs");
    shadowMap = LoadRenderTexture(2048, 2048);
    SetTextureFilter(shadowMap.depth, TEXTURE_FILTER_BILINEAR);

    shadowMVPLoc = GetShaderLocation(fogShader, "shadowMVP");
    shadowMapTexLoc = GetShaderLocation(fogShader, "shadowMap");
    shadowStrengthLoc = GetShaderLocation(fogShader, "shadowStrength");
    exposureLoc = GetShaderLocation(fogShader, "exposure");
    saturationLoc = GetShaderLocation(fogShader, "saturation");
    SetShaderValue(fogShader, exposureLoc, &toneExposure, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader, saturationLoc, &toneSaturation, SHADER_UNIFORM_FLOAT);

    int smLoc = GetShaderLocation(fogShader, "shadowMap");
    if (smLoc >= 0) {
        int texUnit = 1;
        SetShaderValue(fogShader, smLoc, &texUnit, SHADER_UNIFORM_INT);
    }

    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    debugSystem.Initialize(this, &lua);
    menuSystem = std::make_unique<MenuSystem>(this);
    cmdManager = std::make_unique<CommandManager>(this);
    questSystem = std::make_unique<QuestSystem>();

    if (const char* fromLauncher = std::getenv("EIDOS_PLAYER")) {
        std::string n(fromLauncher);
        if (!n.empty() && n.size() <= 24) {
            player.name = n;
            debugSystem.Log("Signed in as " + n);
        }
    }
    player.inventory.playerName = &player.name;
    player.inventory.appearance = &player.appearance;

    if (Trailer::Enabled()) {
        trailer = std::make_unique<Trailer>();
        trailer->Init(*this);
    }
    else if (AutoShot::Enabled()) {
        autoShot = std::make_unique<AutoShot>();
        autoShot->Init();
    }

    player.inventory.nutrients = player.nutrients;
    player.inventory.satiety = &player.satiety;
    player.inventory.hydration = &player.hydration;
    player.inventory.health = &player.health;

    lua.set_function("locate", &EidosEngine::Locate, this);
    lua.set_function("wind", [this](sol::object arg) {
        if (arg.is<float>() || arg.is<int>()) {
            float f = std::clamp(arg.as<float>(), 0.0f, 30.0f);
            windSystem.SetOverride(f);
            debugSystem.Log("Wind force locked at " + std::to_string((int)f) + " m/s");
        }
        else {
            windSystem.ClearOverride();
            debugSystem.Log("Wind back to automatic (biome-driven)");
        }
        });
    lua.set_function("geology", [this]() {
        int px = (int)floorf(player.camera.position.x);
        int pz = (int)floorf(player.camera.position.z);
        ColumnInfo col = worldGen.GetColumnInfo(px, pz);
        const RockColumn& rc = col.rock;

        debugSystem.Log("--- Geology at " + std::to_string(px) + ", " + std::to_string(pz) + " ---");
        debugSystem.Log("Province: " + std::string(WorldGenerator::RockSuiteName(rc.suite)));
        debugSystem.Log("Surface rock: " + BlockInfo::GetName((int)worldGen.GetSurfaceRock(px, pz)));

        if (rc.veinActive)
            debugSystem.Log("Vein: " + BlockInfo::GetName((int)rc.veinRock) +
                " from y=" + std::to_string(rc.veinBottom) +
                " to y=" + std::to_string(rc.veinTop));

        std::string strata;
        int lastId = -1;
        for (int y = std::min(col.height - 1, 150); y >= 1; --y) {
            BlockType b = worldGen.GetBlockFast(px, y, pz, col);
            int id = (int)b;
            if (id < 16 || id > 45 || id == 17 || id == 18 || id == 19) continue;
            if (id == lastId) continue;
            lastId = id;
            if (!strata.empty()) strata += " / ";
            strata += BlockInfo::GetName(id) + " y" + std::to_string(y);
            if (strata.size() > 150) { strata += " ..."; break; }
        }
        debugSystem.Log("Strata: " + strata);
        });
    lua.set_function("gamerule", [this](sol::object nameArg, sol::object valueArg) {
        if (!nameArg.is<std::string>()) {
            debugSystem.Log("Usage: gamerule randomTickSpeed <n>");
            debugSystem.Log("  randomTickSpeed = " + std::to_string(randomTickSpeed) +
                " (vanilla default 3, 0 freezes the world)");
            return;
        }

        std::string rule = nameArg.as<std::string>();
        std::transform(rule.begin(), rule.end(), rule.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        if (rule != "randomtickspeed") {
            debugSystem.Log("Unknown gamerule '" + rule + "'. Known: randomTickSpeed");
            return;
        }

        if (!valueArg.valid()) {
            debugSystem.Log("randomTickSpeed = " + std::to_string(randomTickSpeed));
            return;
        }

        int v = 3;
        if (valueArg.is<int>())        v = valueArg.as<int>();
        else if (valueArg.is<double>()) v = (int)valueArg.as<double>();
        else if (valueArg.is<std::string>()) {
            try { v = std::stoi(valueArg.as<std::string>()); }
            catch (...) {
                debugSystem.Log("randomTickSpeed needs a whole number");
                return;
            }
        }

        randomTickSpeed = std::clamp(v, 0, 2000);
        debugSystem.Log("randomTickSpeed = " + std::to_string(randomTickSpeed) +
            (randomTickSpeed == 0 ? "  (growth and decay frozen)"
                : randomTickSpeed > 3 ? "  (world sped up)" : ""));
        });
    lua.set_function("save", [this]() { this->SaveWorld(false); });
    lua.set_function("exit", [this]() { this->appRunning = false; });
    lua.set_function("time", [this](std::string action, std::string value) {
        if (action != "set") {
            debugSystem.Log("Usage: time set <day|noon|night|midnight|sunrise|sunset|0-24>");
            return;
        }

        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        float t = -1.0f;
        if (value == "day")           t = 0.36f;
        else if (value == "noon")     t = 0.50f;
        else if (value == "sunrise")  t = 0.25f;
        else if (value == "sunset")   t = 0.75f;
        else if (value == "night")    t = 0.88f;
        else if (value == "midnight") t = 0.00f;
        else {
            try {
                float raw = std::stof(value);
                t = (raw > 1.0f) ? (raw / 24.0f) : raw;
            }
            catch (...) {
                debugSystem.Log("Unknown time '" + value + "'");
                return;
            }
        }

        t = t - floorf(t);
        skySystem.SetTime(t);
        debugSystem.Log("Time set to " + std::to_string((int)(t * 24.0f)) + ":00");
        });
    lua.set_function("gamemode", [this](int mode) { if (mode == 0)player.currentMode = GameMode::Survival; if (mode == 1)player.currentMode = GameMode::Creative; if (mode == 2)player.currentMode = GameMode::Spectator; });
    lua.set_function("gm", [this](int mode) { if (mode == 0)player.currentMode = GameMode::Survival; if (mode == 1)player.currentMode = GameMode::Creative; if (mode == 2)player.currentMode = GameMode::Spectator; });
    cmdManager->BindCommands(lua);

    for (int i = 0; i < 6; i++) panoramaTextures[i] = { 0 };
    LoadPanorama();

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
        int safeX = 0, safeZ = 12000;
        int h = worldGen.GetHeight(safeX, safeZ);
        player.position = { (float)safeX + 0.5f, (float)h + 25.0f, (float)safeZ + 0.5f };
    }

    player.camera.position = player.position;
    player.camera.target = { player.position.x + 1.0f, player.position.y, player.position.z };
    player.camera.up = { 0.0f, 1.0f, 0.0f };

    unsigned int hwThreads = std::thread::hardware_concurrency();
    if (hwThreads == 0) hwThreads = 4;

    unsigned int workers = (hwThreads > 2) ? hwThreads - 1 : 1;
    unsigned int terrainT = std::max(1u, workers * 2 / 5);
    unsigned int meshT = std::max(1u, workers - terrainT);

    debugSystem.Log("Workers: " + std::to_string(terrainT) + " terrain, " +
        std::to_string(meshT) + " mesh (" + std::to_string(hwThreads) + " cores)");

    for (unsigned int i = 0; i < terrainT; i++) threadPool.emplace_back(&EidosEngine::TerrainWorker, this);
    for (unsigned int i = 0; i < meshT; i++) threadPool.emplace_back(&EidosEngine::MeshWorker, this);

    UpdateChunks();
}

EidosEngine::~EidosEngine() {
    if (currentState == GameState::Playing || currentState == GameState::Paused) SaveWorld(false);
    SaveConfig();
    appRunning = false;
    for (std::thread& t : threadPool) if (t.joinable()) t.join();
    UnloadShader(fogShader);
    UnloadShader(waterShader);
    UnloadShader(shadowShader);
    UnloadRenderTexture(shadowMap);
    UnloadPanorama();
    UnloadWorld();
    CloseWindow();
}
