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

