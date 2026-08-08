#include "EidosEngine.h"
#include "../World/Chunk.h"
#include <algorithm>
#include <cmath>

namespace {

    unsigned int NextRand(unsigned int& s) {
        s = s * 1664525u + 1013904223u;
        return s >> 8;
    }

    bool SmothersGrass(BlockType t) {
        switch (t) {
        case BlockType::Air:
        case BlockType::Glass:
        case BlockType::TallGrass:
        case BlockType::Rose:
        case BlockType::Dandelion:
        case BlockType::DeadBush:
        case BlockType::BrownMushroom:
        case BlockType::RedMushroom:
        case BlockType::OakSapling:
        case BlockType::OakLeaves:
        case BlockType::SpruceLeaves:
        case BlockType::BirchLeaves:
        case BlockType::AcaciaLeaves:
        case BlockType::JungleLeaves:
            return false;
        default:
            return true;
        }
    }

    bool IsAnyLeaves(BlockType t) {
        return t == BlockType::OakLeaves || t == BlockType::SpruceLeaves ||
            t == BlockType::BirchLeaves || t == BlockType::AcaciaLeaves ||
            t == BlockType::JungleLeaves;
    }

    bool IsAnyLog(BlockType t) {
        return t == BlockType::OakLog || t == BlockType::SpruceLog ||
            t == BlockType::BirchLog || t == BlockType::AcaciaLog ||
            t == BlockType::JungleLog;
    }
}

void EidosEngine::UpdateClimate(float dt) {
    climateTimer += dt;
    if (climateTimer < 0.25f) return;
    float step = climateTimer;
    climateTimer = 0.0f;

    int px = (int)floorf(player.position.x);
    int py = (int)floorf(player.position.y);
    int pz = (int)floorf(player.position.z);

    int surfaceY = worldGen.GetHeight(px, pz);
    ClimateInfo ci = worldGen.GetClimate(px, py, pz, skySystem.GetTime(), surfaceY);

    float fire = 0.0f;
    unsigned char packed = 0;
    if (Chunk* c = GetChunkAt(px, pz)) {
        int lx = (px % 16 + 16) % 16;
        int lz = (pz % 16 + 16) % 16;
        packed = c->GetLight(lx, std::clamp(py, 0, 255), lz);
        fire = (float)Chunk::BlockLightOf(packed) / 15.0f;
    }
    player.warmthFromFire = fire;

    float felt = ci.airC + fire * 16.0f;

    Vector3 wv = windSystem.GetWindVector();
    float wind = sqrtf(wv.x * wv.x + wv.y * wv.y + wv.z * wv.z);
    if (felt < 20.0f) felt -= wind * 0.35f;

    if (Chunk::IsWaterBlock(GetBlockAt(px, py, pz))) felt -= 9.0f;

    float target = 36.6f + (felt - 19.0f) * 0.22f;
    target = std::clamp(target, 28.0f, 43.0f);
    player.bodyTempC += (target - player.bodyTempC) * std::min(1.0f, step * 0.09f);

    Inventory::EnvReadout& e = player.inventory.env;
    e.airC = ci.airC;
    e.feltC = felt;
    e.bodyC = player.bodyTempC;
    e.baseC = ci.baseC;
    e.altitudeC = ci.altitudeC;
    e.diurnalC = ci.diurnalC;
    e.humidity = ci.humidity;
    e.windSpeed = wind;
    e.timeOfDay = skySystem.GetTime();
    e.altitude = py;
    e.lightSky = Chunk::SkyLightOf(packed);
    e.lightBlock = Chunk::BlockLightOf(packed);
    e.underground = ci.underground;
    e.biome = worldGen.GetBiomeName(px, pz);
}

void EidosEngine::UpdateWorldTicks(float dt) {
    if (randomTickSpeed <= 0) return;

    worldTickTimer += dt;
    if (worldTickTimer < 0.3f) return;
    worldTickTimer = 0.0f;

    const int BASE_TICK_SPEED = 3;
    const int MAX_TARGETS = 4096;
    int budget = 48 * randomTickSpeed / BASE_TICK_SPEED;
    budget = std::clamp(budget, 1, MAX_TARGETS);

    struct Target { int x, y, z; };
    static Target targets[MAX_TARGETS];
    int found = 0;

    {
        std::lock_guard<std::recursive_mutex> lock(chunkListMutex);
        if (chunks.empty()) return;

        int pcx = (int)floorf(player.position.x / 16.0f);
        int pcz = (int)floorf(player.position.z / 16.0f);

        for (int i = 0; i < budget; i++) {
            const auto& c = chunks[NextRand(worldTickSeed) % chunks.size()];
            if (c->state < 3) continue;
            if (std::abs(c->chunkX - pcx) > 6 || std::abs(c->chunkZ - pcz) > 6) continue;

            int lx = (int)(NextRand(worldTickSeed) & 15);
            int lz = (int)(NextRand(worldTickSeed) & 15);
            int span = std::max(2, c->maxY - 1);
            int ly = 2 + (int)(NextRand(worldTickSeed) % (unsigned int)span);

            targets[found++] = { c->chunkX * 16 + lx, ly, c->chunkZ * 16 + lz };
        }
    }

    for (int i = 0; i < found; i++)
        TickBlock(targets[i].x, targets[i].y, targets[i].z);
}

void EidosEngine::TickBlock(int x, int y, int z) {
    BlockType b = GetBlockAt(x, y, z);
    if (b == BlockType::Air) return;

    BlockType above = GetBlockAt(x, y + 1, z);
    unsigned int roll = NextRand(worldTickSeed) % 1000u;

    switch (b) {
    case BlockType::Grass: {
        if (SmothersGrass(above)) {
            SetBlockGlobal(x, y, z, (int)BlockType::Dirt);
            return;
        }
        if (roll < 260) {
            int dx = (int)(NextRand(worldTickSeed) % 3u) - 1;
            int dz = (int)(NextRand(worldTickSeed) % 3u) - 1;
            int dy = (int)(NextRand(worldTickSeed) % 3u) - 1;
            if (dx == 0 && dz == 0) return;
            int nx = x + dx, ny = y + dy, nz = z + dz;
            if (GetBlockAt(nx, ny, nz) == BlockType::Dirt &&
                !SmothersGrass(GetBlockAt(nx, ny + 1, nz)))
                SetBlockGlobal(nx, ny, nz, (int)BlockType::Grass);
        }
        return;
    }
    case BlockType::TallGrass: {
        if (roll < 30) {
            SetBlockGlobal(x, y, z, (int)BlockType::Air);
            return;
        }
        if (roll < 120) {
            int dx = (int)(NextRand(worldTickSeed) % 3u) - 1;
            int dz = (int)(NextRand(worldTickSeed) % 3u) - 1;
            if (dx == 0 && dz == 0) return;
            int nx = x + dx, nz = z + dz;
            if (GetBlockAt(nx, y, nz) == BlockType::Air &&
                GetBlockAt(nx, y - 1, nz) == BlockType::Grass)
                SetBlockGlobal(nx, y, nz, (int)BlockType::TallGrass);
        }
        return;
    }
    case BlockType::Rose:
    case BlockType::Dandelion: {
        if (roll < 18) SetBlockGlobal(x, y, z, (int)BlockType::Air);
        return;
    }
    case BlockType::OakSapling: {
        if (roll < 70) TryGrowOak(x, y, z);
        return;
    }
    case BlockType::BerryBush: {
        BlockType ground = GetBlockAt(x, y - 1, z);
        if (ground != BlockType::Grass && ground != BlockType::Dirt &&
            ground != BlockType::CoarseDirt) {
            SetBlockGlobal(x, y, z, (int)BlockType::Air);
            return;
        }
        if (roll < 45) SetBlockGlobal(x, y, z, (int)BlockType::BerryBushRipe);
        return;
    }
    case BlockType::BerryBushRipe: {
        if (roll < 90) {
            int dx = (int)(NextRand(worldTickSeed) % 5u) - 2;
            int dz = (int)(NextRand(worldTickSeed) % 5u) - 2;
            if (dx == 0 && dz == 0) return;
            int nx = x + dx, nz = z + dz;
            BlockType ground = GetBlockAt(nx, y - 1, nz);
            if (GetBlockAt(nx, y, nz) == BlockType::Air &&
                (ground == BlockType::Grass || ground == BlockType::Dirt))
                SetBlockGlobal(nx, y, nz, (int)BlockType::BerryBush);
        }
        return;
    }
    default:
        break;
    }

    if (IsAnyLeaves(b)) {
        if (!HasLogNearby(x, y, z)) {
            if (roll < 500) SetBlockGlobal(x, y, z, (int)BlockType::Air);
            return;
        }
        if (b == BlockType::OakLeaves && roll < 8) TryDropAcorn(x, y, z);
    }
}

bool EidosEngine::HasLogNearby(int x, int y, int z) {
    for (int dy = -3; dy <= 3; dy++)
        for (int dx = -3; dx <= 3; dx++)
            for (int dz = -3; dz <= 3; dz++)
                if (IsAnyLog(GetBlockAt(x + dx, y + dy, z + dz))) return true;
    return false;
}

void EidosEngine::TryDropAcorn(int x, int y, int z) {
    int gx = x + (int)(NextRand(worldTickSeed) % 5u) - 2;
    int gz = z + (int)(NextRand(worldTickSeed) % 5u) - 2;

    for (int dy = 1; dy <= 14; dy++) {
        int gy = y - dy;
        if (gy < 2) return;
        BlockType ground = GetBlockAt(gx, gy, gz);
        if (ground == BlockType::Air) continue;
        if ((ground == BlockType::Grass || ground == BlockType::Dirt) &&
            GetBlockAt(gx, gy + 1, gz) == BlockType::Air)
            SetBlockGlobal(gx, gy + 1, gz, (int)BlockType::OakSapling);
        return;
    }
}

void EidosEngine::TryGrowOak(int x, int y, int z) {
    int h = 4 + (int)(NextRand(worldTickSeed) % 3u);

    for (int i = 1; i < h; i++)
        if (GetBlockAt(x, y + i, z) != BlockType::Air) return;

    for (int i = 0; i < h; i++)
        SetBlockGlobal(x, y + i, z, (int)BlockType::OakLog);

    int top = y + h;
    for (int ly = top - 2; ly <= top; ly++) {
        int r = (ly == top) ? 1 : 2;
        for (int dx = -r; dx <= r; dx++) {
            for (int dz = -r; dz <= r; dz++) {
                if (dx == 0 && dz == 0 && ly < top) continue;
                if (std::abs(dx) == r && std::abs(dz) == r && (NextRand(worldTickSeed) & 1u)) continue;
                if (GetBlockAt(x + dx, ly, z + dz) == BlockType::Air)
                    SetBlockGlobal(x + dx, ly, z + dz, (int)BlockType::OakLeaves);
            }
        }
    }
}
