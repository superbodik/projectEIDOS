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
