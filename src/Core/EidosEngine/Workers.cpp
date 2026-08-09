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

