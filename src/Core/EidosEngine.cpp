#include "EidosEngine.h"
#include <unordered_map>
#include "CommandManager.h"
#include "../Cinematic/Trailer.h"
#include "../Cinematic/AutoShot.h"
#include "../Inventory/BlockInfo.h"
#include "../Progression/QuestSystem.h"
#include "../World/Chunk.h"
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

static float PanoHash(int a, int b) {
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFFu) / 65535.0f;
}

static float PanoRidge(float az, float seedOff, float scale) {
    float v = 0.0f;
    v += sinf(az * 2.0f + seedOff) * 0.45f;
    v += sinf(az * 5.0f + seedOff * 1.7f) * 0.26f;
    v += sinf(az * 11.0f + seedOff * 2.3f) * 0.16f;
    v += sinf(az * 23.0f + seedOff * 3.1f) * 0.08f;
    return v * scale;
}

void EidosEngine::LoadPanorama() {
    for (int i = 0; i < 6; i++) {
        if (panoramaTextures[i].id != 0) {
            UnloadTexture(panoramaTextures[i]);
            panoramaTextures[i] = { 0 };
        }
    }

    const int S = 512;
    const Vector3 sunDir = Vector3Normalize({ 0.62f, 0.20f, -0.76f });

    auto faceDir = [](int f, float u, float v) -> Vector3 {
        switch (f) {
        case 0:  return Vector3Normalize({ 1.0f, 1.0f - 2.0f * v, -1.0f + 2.0f * u });
        case 1:  return Vector3Normalize({ -1.0f, 1.0f - 2.0f * v, 1.0f - 2.0f * u });
        case 2:  return Vector3Normalize({ 1.0f - 2.0f * u, 1.0f, -1.0f + 2.0f * v });
        case 3:  return Vector3Normalize({ 1.0f - 2.0f * u, -1.0f, 1.0f - 2.0f * v });
        case 4:  return Vector3Normalize({ 1.0f - 2.0f * u, 1.0f - 2.0f * v, 1.0f });
        default: return Vector3Normalize({ -1.0f + 2.0f * u, 1.0f - 2.0f * v, -1.0f });
        }
        };

    for (int f = 0; f < 6; f++) {
        Image img = GenImageColor(S, S, BLACK);

        for (int py = 0; py < S; py++) {
            for (int px = 0; px < S; px++) {
                Vector3 d = faceDir(f, ((float)px + 0.5f) / S, ((float)py + 0.5f) / S);
                float el = d.y;
                float az = atan2f(d.z, d.x);

                float t = std::clamp(el * 1.4f + 0.12f, 0.0f, 1.0f);
                Color zenith = { 26, 52, 104, 255 };
                Color mid = { 74, 126, 186, 255 };
                Color horizon = { 214, 168, 132, 255 };
                float r, g, b;
                if (t < 0.42f) {
                    float k = t / 0.42f;
                    r = horizon.r + (mid.r - horizon.r) * k;
                    g = horizon.g + (mid.g - horizon.g) * k;
                    b = horizon.b + (mid.b - horizon.b) * k;
                }
                else {
                    float k = (t - 0.42f) / 0.58f;
                    r = mid.r + (zenith.r - mid.r) * k;
                    g = mid.g + (zenith.g - mid.g) * k;
                    b = mid.b + (zenith.b - mid.b) * k;
                }

                float sunDot = d.x * sunDir.x + d.y * sunDir.y + d.z * sunDir.z;
                if (sunDot > 0.0f) {
                    float glow = powf(sunDot, 42.0f) * 0.85f + powf(sunDot, 6.0f) * 0.22f;
                    r += 255.0f * glow; g += 232.0f * glow; b += 176.0f * glow;
                }

                if (el > 0.22f) {
                    float sx = d.x / (fabsf(d.y) + 0.001f);
                    float sz = d.z / (fabsf(d.y) + 0.001f);
                    int gx = (int)floorf(sx * 46.0f), gz = (int)floorf(sz * 46.0f);
                    float sp = PanoHash(gx, gz);
                    if (sp > 0.9955f) {
                        float tw = 0.55f + PanoHash(gz, gx) * 0.45f;
                        float fade = std::clamp((el - 0.22f) * 2.6f, 0.0f, 1.0f);
                        float a = tw * fade * 190.0f;
                        r += a; g += a; b += a * 0.94f;
                    }
                }

                struct Layer { float base, scale, dist; Color tone; };
                const Layer layers[4] = {
                    { 0.175f, 0.062f, 0.80f, { 178, 194, 216, 255 } },
                    { 0.132f, 0.082f, 0.62f, { 140, 158, 186, 255 } },
                    { 0.088f, 0.104f, 0.40f, {  98, 114, 142, 255 } },
                    { 0.038f, 0.128f, 0.16f, {  64,  76,  98, 255 } },
                };

                float sunAz = atan2f(sunDir.z, sunDir.x);

                for (int L = 0; L < 4; L++) {
                    float ridge = layers[L].base + PanoRidge(az, 1.3f * (float)(L + 1), layers[L].scale);
                    if (el >= ridge) continue;

                    float depth = std::clamp((ridge - el) * 6.0f, 0.0f, 1.0f);
                    Color tn = layers[L].tone;

                    float facing = cosf(az - sunAz);
                    float lit = 0.80f + 0.38f * std::clamp(facing, -1.0f, 1.0f);
                    float tr = tn.r * lit, tg = tn.g * lit * 0.99f, tb = tn.b * lit * 0.96f;
                    if (facing > 0.0f) {
                        float warm = facing * facing * 0.30f;
                        tr += 46.0f * warm; tg += 26.0f * warm; tb -= 6.0f * warm;
                    }

                    if (ridge > 0.100f && el > ridge - 0.030f - PanoRidge(az, 7.7f, 0.012f)) {
                        float cap = std::clamp((el - (ridge - 0.034f)) * 32.0f, 0.0f, 1.0f);
                        tr += (236.0f - tr) * cap; tg += (242.0f - tg) * cap; tb += (250.0f - tb) * cap;
                    }

                    tr *= (1.0f - depth * 0.22f);
                    tg *= (1.0f - depth * 0.22f);
                    tb *= (1.0f - depth * 0.20f);

                    float haze = 1.0f - layers[L].dist * (1.0f - depth * 0.5f);
                    r = tr + (r - tr) * (1.0f - haze);
                    g = tg + (g - tg) * (1.0f - haze);
                    b = tb + (b - tb) * (1.0f - haze);
                }

                if (el < -0.05f) {
                    float k = std::clamp((-el - 0.05f) * 2.6f, 0.0f, 1.0f);
                    r += (30.0f - r) * k; g += (36.0f - g) * k; b += (48.0f - b) * k;
                }

                ImageDrawPixel(&img, px, py, {
                    (unsigned char)std::clamp(r, 0.0f, 255.0f),
                    (unsigned char)std::clamp(g, 0.0f, 255.0f),
                    (unsigned char)std::clamp(b, 0.0f, 255.0f), 255 });
            }
        }

        panoramaTextures[f] = LoadTextureFromImage(img);
        SetTextureFilter(panoramaTextures[f], TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(panoramaTextures[f], TEXTURE_WRAP_CLAMP);
        UnloadImage(img);
    }

    hasPanorama = true;
}

void EidosEngine::UnloadPanorama() {
    for (int i = 0; i < 6; i++) {
        if (panoramaTextures[i].id != 0) {
            UnloadTexture(panoramaTextures[i]);
            panoramaTextures[i] = { 0 };
        }
    }
    hasPanorama = false;
}

void EidosEngine::DrawPanorama() {
    if (!hasPanorama) return;

    rlDisableDepthMask();
    rlDisableDepthTest();
    rlDisableBackfaceCulling();

    Vector3 p = player.camera.position;
    float s = 50.0f;

    auto drawFace = [](Texture2D tex, Vector3 tl, Vector3 bl, Vector3 br, Vector3 tr) {
        if (tex.id == 0) return;
        rlSetTexture(tex.id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(tl.x, tl.y, tl.z);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(tr.x, tr.y, tr.z);
        rlEnd();
        rlSetTexture(0);
        };

    drawFace(panoramaTextures[0], { p.x + s, p.y + s, p.z - s }, { p.x + s, p.y - s, p.z - s }, { p.x + s, p.y - s, p.z + s }, { p.x + s, p.y + s, p.z + s });
    drawFace(panoramaTextures[1], { p.x - s, p.y + s, p.z + s }, { p.x - s, p.y - s, p.z + s }, { p.x - s, p.y - s, p.z - s }, { p.x - s, p.y + s, p.z - s });
    drawFace(panoramaTextures[2], { p.x + s, p.y + s, p.z - s }, { p.x + s, p.y + s, p.z + s }, { p.x - s, p.y + s, p.z + s }, { p.x - s, p.y + s, p.z - s });
    drawFace(panoramaTextures[3], { p.x + s, p.y - s, p.z + s }, { p.x + s, p.y - s, p.z - s }, { p.x - s, p.y - s, p.z - s }, { p.x - s, p.y - s, p.z + s });
    drawFace(panoramaTextures[4], { p.x + s, p.y + s, p.z + s }, { p.x + s, p.y - s, p.z + s }, { p.x - s, p.y - s, p.z + s }, { p.x - s, p.y + s, p.z + s });
    drawFace(panoramaTextures[5], { p.x - s, p.y + s, p.z - s }, { p.x - s, p.y - s, p.z - s }, { p.x + s, p.y - s, p.z - s }, { p.x + s, p.y + s, p.z - s });

    rlEnableBackfaceCulling();
    rlEnableDepthTest();
    rlEnableDepthMask();
}

void EidosEngine::LoadConfig() {
    std::string path = "config/settings.ini";
    if (!fs::exists("config")) fs::create_directories("config");
    if (fs::exists(path)) {
        std::ifstream file(path); std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line); std::string key; ss >> key;
            if (key == "RenderDistance") { int val; if (ss >> val) SetRenderDistance(val); }
            else if (key == "FOV") { float val; if (ss >> val) SetFOV(val); }
            else if (key == "MaxFPS") { int val; if (ss >> val) SetMaxFPS(val); }
            else if (key == "ShowFPS") { int val; if (ss >> val) showSimpleFPS = (val != 0); }
            else if (key == "ShowWind") { int val; if (ss >> val) showWind = (val != 0); }
            else if (key == "Fullscreen") { int val; if (ss >> val && val == 1) ToggleFullscreen(); }
            else if (key == "WindowW") { int val; if (ss >> val && val >= 854) windowedW = val; }
            else if (key == "WindowH") { int val; if (ss >> val && val >= 480) windowedH = val; }
            else if (key == "GuiScale") { int val; if (ss >> val && val >= 0 && val <= 3) OverlayUI::guiScaleSetting = val; }
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
        file << "ShowWind " << (showWind ? 1 : 0) << "\n";
        bool borderless = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        file << "Fullscreen " << (borderless ? 1 : 0) << "\n";
        file << "WindowW " << (borderless ? windowedW : GetScreenWidth()) << "\n";
        file << "WindowH " << (borderless ? windowedH : GetScreenHeight()) << "\n";
        file << "GuiScale " << OverlayUI::guiScaleSetting << "\n";
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
}

int EidosEngine::GetRenderDistance() const { return renderDistance; }
void EidosEngine::SetMaxFPS(int fps) { targetFPS = fps; SetTargetFPS(fps <= 0 ? 0 : fps); }
int EidosEngine::GetMaxFPS() const { return targetFPS; }
void EidosEngine::SetFOV(float fov) { targetFOV = std::clamp(fov, 30.0f, 110.0f); player.camera.fovy = targetFOV; }
float EidosEngine::GetFOV() const { return targetFOV; }
float EidosEngine::GetUIScale() const { return (float)GetScreenHeight() / 1080.0f; }
bool EidosEngine::ShouldClose() const { return WindowShouldClose() || !appRunning; }

void EidosEngine::ToggleFullscreen() {
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        ToggleBorderlessWindowed();
        if (windowedW > 0 && windowedH > 0) SetWindowSize(windowedW, windowedH);
    }
    else {
        if (!IsWindowMaximized()) {
            windowedW = GetScreenWidth();
            windowedH = GetScreenHeight();
        }
        ToggleBorderlessWindowed();
    }
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
        out << rules.Serialize() << "\n";
        out.close();
        if (!autoSave) debugSystem.Log("Saved level.dat");
    }
    questSystem->Save(path);

    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    for (auto& c : chunks) { c->SaveToFile(path); }
}

void EidosEngine::LoadWorld(std::string worldName) {
    currentWorldName = worldName;
    std::string path = "saves/" + currentWorldName;
    questSystem->Load(path);

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

            std::string rest;
            std::getline(in, rest);
            std::string rulesLine;
            while (std::getline(in, rulesLine)) {
                if (rulesLine.rfind("rules", 0) == 0) break;
            }
            if (!rules.Deserialize(rulesLine))
                rules = WorldRules::Preset(Difficulty::Survival);

            ApplyGenSettings();
            player.maxHealth = 1.0f;
            worldDead = false;
            debugSystem.Log("Loaded world: " + worldName +
                "  [" + WorldRules::Name(rules.difficulty) + "]");
        } in.close();
    }
    else {
        int newSeed = GetRandomValue(0, 999999); if (currentState != GameState::CreateWorld) { worldGen.SetSeed(newSeed); }

        int safeX = 0, safeZ = 2600;
        const BiomeType wanted[] = {
            BiomeType::TemperateDeciduousForest,
            BiomeType::TemperateRainforest,
            BiomeType::Savanna,
            BiomeType::Taiga
        };
        for (BiomeType want : wanted) {
            BiomeSearchResult r = worldGen.FindBiome(0, 2600, want, 6000, 96);
            if (!r.found) continue;
            if (worldGen.GetHeight(r.x, r.z) <= WorldGenerator::SEA_LEVEL + 1) continue;
            safeX = r.x;
            safeZ = r.z;
            debugSystem.Log("Spawn: " + worldGen.GetBiomeName(safeX, safeZ) +
                " at " + std::to_string(safeX) + ", " + std::to_string(safeZ));
            break;
        }

        int h = worldGen.GetHeight(safeX, safeZ);

        int spawnY = h + 2;
        for (int attempts = 0; attempts < 10; attempts++) {
            BlockType spawnBlock = GetBlockAt(safeX, spawnY - 1, safeZ);
            if (spawnBlock != BlockType::Water && spawnBlock != BlockType::WaterSource) break;
            spawnY += 3;
            safeX += GetRandomValue(-5, 5);
            safeZ += GetRandomValue(-5, 5);
            h = worldGen.GetHeight(safeX, safeZ);
            spawnY = h + 2;
        }
        player.position = { (float)safeX + 0.5f, (float)spawnY, (float)safeZ + 0.5f }; player.velocity = { 0,0,0 };
        debugSystem.Log("Starting world: " + worldName);
    }
    for (int i = 0; i < 36; i++) player.inventory.slots[i] = { 0,0 };
    player.inventory.slots[0] = { 6, 64 }; player.inventory.slots[1] = { 17, 64 }; player.inventory.slots[2] = { 100, 64 };
    player.inventory.slots[3] = { 18, 64 }; player.inventory.slots[4] = { 3, 64 };
    autoSaveTimer = 0.0f;

    player.camera.position = player.position;
    player.camera.target = { player.position.x + 1.0f, player.position.y, player.position.z };

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

                    bool propagate = wasFirstBuild;
                    if (lightChanged && !wasFirstBuild)
                        propagate = (task->lightPasses.fetch_add(1) < 3);

                    if (propagate) {
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
    bool isMenuState = (currentState == GameState::MainMenu || currentState == GameState::Settings || currentState == GameState::WorldSelect || currentState == GameState::CreateWorld);

    if (isMenuState && hasPanorama) return;

    int pChunkX = (int)floor(player.camera.position.x / Chunk::CHUNK_SIZE_X);
    int pChunkZ = (int)floor(player.camera.position.z / Chunk::CHUNK_SIZE_Z);
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
    for (auto& c : chunks) { if (c->state == 2 && uploads < 200) { c->UploadMeshGPU(); uploads++; } }

    {
        static double lastReport = 0.0;
        double now = GetTime();
        if (now - lastReport > 0.5) {
            lastReport = now;

            int s0 = 0, s1 = 0, s2 = 0, s3 = 0, busy = 0;
            for (auto& c : chunks) {
                switch (c->state.load()) {
                case 0: s0++; break;
                case 1: s1++; break;
                case 2: s2++; break;
                default: s3++; break;
                }
                if (c->isBusy) busy++;
            }

            size_t tq = 0, mq = 0;
            { std::lock_guard<std::mutex> l(terrainQueueMutex); tq = terrainQueue.size(); }
            { std::lock_guard<std::mutex> l(meshQueueMutex);    mq = meshQueue.size(); }

            const char* what =
                (tq > 0 || s0 > 0) ? "generating terrain" :
                (mq > 0 || s1 > 0) ? "building meshes" :
                (s2 > 0)           ? "uploading to GPU" :
                                     "idle";

            printf("[gen] %-18s chunks %4d | empty %3d terrain %3d mesh %3d live %4d "
                   "| queue t=%3zu m=%3zu | busy %3d | uploaded %3d\n",
                what, (int)chunks.size(), s0, s1, s2, s3, tq, mq, busy, uploads);
            fflush(stdout);
        }
    }

    std::unordered_map<long long, std::shared_ptr<Chunk>> byPos;
    byPos.reserve(chunks.size() * 2);
    for (auto& c : chunks)
        byPos[((long long)c->chunkX << 32) ^ (unsigned int)c->chunkZ] = c;

    {
        std::vector<std::pair<int, std::shared_ptr<Chunk>>> pending;
        for (auto& c : chunks) {
            if (c->isBusy || !(c->dirty || c->state == 1)) continue;
            int dx = c->chunkX - pChunkX;
            int dz = c->chunkZ - pChunkZ;
            pending.emplace_back(dx * dx + dz * dz, c);
        }

        std::sort(pending.begin(), pending.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        std::lock_guard<std::mutex> qLock(meshQueueMutex);
        for (auto& pc : pending) {
            pc.second->isBusy = true;
            meshQueue.push(pc.second);
        }
    }

    size_t tqSize = 0; { std::lock_guard<std::mutex> qLock(terrainQueueMutex); tqSize = terrainQueue.size(); }
    if (tqSize > 100) return;

    int scheduled = 0;
    int genDist = renderDistance + 1;

    for (int d = 0; d <= genDist; d++) {
        for (int x = -d; x <= d; x++) {
            for (int z = -d; z <= d; z++) {
                if (abs(x) != d && abs(z) != d) continue;

                int targetX = pChunkX + x;
                int targetZ = pChunkZ + z;

                bool exists = false;
                auto found = byPos.find(((long long)targetX << 32) ^ (unsigned int)targetZ);
                if (found != byPos.end()) {
                    exists = true;
                    std::shared_ptr<Chunk>& c = found->second;
                    if (c->state == 0 && !c->isBusy) {
                        c->isBusy = true;
                        std::lock_guard<std::mutex> qLock(terrainQueueMutex);
                        terrainQueue.push(c);
                    }
                }
                if (!exists) {
                    auto newChunk = std::make_shared<Chunk>(targetX, targetZ);
                    newChunk->isBusy = true; chunks.push_back(newChunk);
                    std::lock_guard<std::mutex> qLock(terrainQueueMutex);
                    terrainQueue.push(newChunk);

                    scheduled++;
                    if (scheduled >= 16) return;
                }
            }
        }
    }
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
                (TryEatHeldItem() || TryShapeClay());
            bool ate = used;

            if (!ate && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
                RayHitInfo hit = CastRay(5.0f);
                if (hit.hit) {
                    int targetX = (int)floor(hit.x);
                    int targetY = (int)floor(hit.y);
                    int targetZ = (int)floor(hit.z);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        int broken = (int)GetBlockAt(targetX, targetY, targetZ);
                        GrantForage(broken, targetX, targetY, targetZ);

                        if (broken == (int)BlockType::BerryBushRipe) {
                            SetBlockGlobal(targetX, targetY, targetZ, (int)BlockType::BerryBush);
                        }
                        else {
                            if (broken != 0 && player.currentMode == GameMode::Survival)
                                player.inventory.AddItem(broken, 1);
                            SetBlockGlobal(targetX, targetY, targetZ, 0);
                        }
                    }
                    else {
                        int pX = (int)floor(player.position.x); int pY = (int)floor(player.position.y); int pZ = (int)floor(player.position.z);
                        if ((hit.px != pX || hit.pz != pZ) || (hit.py != pY && hit.py != pY + 1)) {
                            int id = player.inventory.GetSelectedBlockID();
                            if (id >= (int)BlockType::Berries) id = 0;
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

std::string EidosEngine::GetBiomeName(int x, int y, int z) { (void)y; return worldGen.GetBiomeName(x, z); }
std::string EidosEngine::GetBlockName(int type) const { return BlockInfo::GetName(type); }

void EidosEngine::SetBlockGlobal(int x, int y, int z, int type) {
    if (type != 0) {
        int px = (int)floor(player.position.x);
        int py = (int)floor(player.position.y);
        int pz = (int)floor(player.position.z);
        if (x == px && z == pz && (y == py || y == py + 1)) return;
    }
    std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
    if (Chunk* c = GetChunkAt(x, z)) {
        int lx = (x % 16 + 16) % 16;
        int lz = (z % 16 + 16) % 16;
        c->SetBlock(lx, y, lz, type);

        if (lx == 0 && GetChunkAt(x - 1, z))  GetChunkAt(x - 1, z)->dirty = true;
        if (lx == 15 && GetChunkAt(x + 1, z)) GetChunkAt(x + 1, z)->dirty = true;
        if (lz == 0 && GetChunkAt(x, z - 1))  GetChunkAt(x, z - 1)->dirty = true;
        if (lz == 15 && GetChunkAt(x, z + 1)) GetChunkAt(x, z + 1)->dirty = true;
    }
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
        BlockType t = GetBlockAt(bx, by, bz);
        if (t != BlockType::Air) {
            info.hit = true; info.x = bx; info.y = by; info.z = bz; info.px = lx; info.py = ly; info.pz = lz; return info;
        }
        lx = bx; ly = by; lz = bz;
    }
    return info;
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
void EidosEngine::SetPlayerPosition(Vector3 p) {
    player.position = p;
}

void EidosEngine::SetSkyTime(float t) {
    skySystem.SetTime(t);
}

void EidosEngine::ApplyGenSettings() {
    WorldGenerator::GenSettings g;
    g.oreDensity = rules.oreDensity;
    g.caveDensity = rules.caveDensity;
    g.latitudeScale = rules.latitudeScale;
    worldGen.SetGenSettings(g);
}

void EidosEngine::SetQuestTreeOpen(bool open) {
    if (!questSystem) return;
    if (questSystem->IsOpen() != open) questSystem->Toggle();
}

bool EidosEngine::IsAreaLoaded(int radius) {
    int cx = (int)floor(player.camera.position.x / Chunk::CHUNK_SIZE_X);
    int cz = (int)floor(player.camera.position.z / Chunk::CHUNK_SIZE_Z);
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
    if (type != "biome") {
        debugSystem.Log("Unknown locate type. Usage: locate biome <Name>");
        debugSystem.Log("Try: locate biome list");
        return;
    }

    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    if (lowered == "list" || lowered == "?" || lowered.empty()) {
        std::string line;
        for (const std::string& n : WorldGenerator::BiomeNames()) {
            if (!line.empty()) line += ", ";
            line += n;
        }
        debugSystem.Log("Biomes: " + line);
        return;
    }

    BiomeType target;
    if (!WorldGenerator::ParseBiome(value, target)) {
        debugSystem.Log("[Err] Unknown biome: " + value);
        debugSystem.Log("Type 'locate biome list' to see all names.");
        return;
    }

    debugSystem.Log("Searching for biome: " + value + "...");

    int px = (int)player.position.x, pz = (int)player.position.z;
    int step = (target == BiomeType::River || target == BiomeType::Beach) ? 16 : 64;
    int range = 32000;
    auto result = worldGen.FindBiome(px, pz, target, range, step);

    if (!result.found) {
        debugSystem.Log("Could not find " + value + " within " + std::to_string(range) + " blocks.");
        return;
    }

    int y = worldGen.GetHeight(result.x, result.z) + 5;
    int dist = (int)sqrtf((float)((result.x - px) * (result.x - px) +
        (result.z - pz) * (result.z - pz)));
    debugSystem.Log("Found " + worldGen.GetBiomeName(result.x, result.z) + " at " +
        std::to_string(result.x) + ", " + std::to_string(result.z) +
        " (" + std::to_string(dist) + " blocks away)");

    player.position = { (float)result.x, (float)y, (float)result.z };
    player.velocity = { 0, 0, 0 };
    LoadWorld(currentWorldName);
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
