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

