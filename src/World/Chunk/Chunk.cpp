#include "../Chunk.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <raymath.h>
#include <algorithm>
#include <queue>
#include <cstring>
#include "rlgl.h"

Texture2D Chunk::atlasTexture = { 0 };
Texture2D Chunk::waterAtlas = { 0 };
Shader Chunk::fogShader = { 0 };
Shader Chunk::waterShader = { 0 };

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    blocks = new BlockType[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    lightMap = new unsigned char[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    mesh = { 0 }; model = { 0 };
    meshTransp = { 0 }; modelTransp = { 0 };
    meshWater = { 0 }; modelWater = { 0 };
}

Chunk::~Chunk() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (hasMesh) UnloadModel(model);
    if (hasMeshTransp) UnloadModel(modelTransp);
    if (hasMeshWater) UnloadModel(modelWater);
    delete[] blocks;
    delete[] lightMap;
}

void Chunk::GenerateTerrain(WorldGenerator& gen) {
    maxY = 0;
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int wx = chunkX * CHUNK_SIZE_X + x;
            int wz = chunkZ * CHUNK_SIZE_Z + z;

            ColumnInfo col = gen.GetColumnInfo(wx, wz);

            int topY = std::max(col.height, col.river.fallTop);
            if (col.height < WorldGenerator::SEA_LEVEL)
                topY = std::max(topY, WorldGenerator::SEA_LEVEL);
            topY = std::max(topY, col.river.waterLevel);
            if (topY > maxY) maxY = topY;

            int ceiling = std::max(col.height + 24, WorldGenerator::SEA_LEVEL);
            ceiling = std::max(ceiling, col.river.fallTop);
            ceiling = std::max(ceiling, col.river.waterLevel);
            if (ceiling > CHUNK_SIZE_Y - 1) ceiling = CHUNK_SIZE_Y - 1;

            for (int y = 0; y <= ceiling; y++) {
                blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] =
                    gen.GetBlockFast(wx, y, wz, col);
            }
            for (int y = ceiling + 1; y < CHUNK_SIZE_Y; y++) {
                blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = BlockType::Air;
            }
        }
    }
    maxY = std::min(maxY + 30, CHUNK_SIZE_Y - 1);
    lightPasses = 0;
    state = 1;
}

unsigned char Chunk::GetLight(int x, int y, int z) {
    int cx = std::clamp(x, 0, 15);
    int cy = std::clamp(y, 0, 255);
    int cz = std::clamp(z, 0, 15);
    return lightMap[cx + CHUNK_SIZE_X * (cz + CHUNK_SIZE_Z * cy)];
}

void Chunk::SetLight(int x, int y, int z, unsigned char level) {
    if (x >= 0 && x < CHUNK_SIZE_X && z >= 0 && z < CHUNK_SIZE_Z && y >= 0 && y < CHUNK_SIZE_Y) {
        lightMap[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = level;
    }
}

BlockType Chunk::GetBlockSafe(int lx, int ly, int lz, Chunk** neighbors) {
    if (ly < 0 || ly >= CHUNK_SIZE_Y) return BlockType::Air;

    int cx = 0, cz = 0;
    if (lx < 0) cx = -1; else if (lx >= CHUNK_SIZE_X) cx = 1;
    if (lz < 0) cz = -1; else if (lz >= CHUNK_SIZE_Z) cz = 1;

    if (cx == 0 && cz == 0) return blocks[lx + CHUNK_SIZE_X * (lz + CHUNK_SIZE_Z * ly)];

    Chunk* n = neighbors[(cx + 1) + (cz + 1) * 3];
    if (n) {
        int nx = (lx % CHUNK_SIZE_X + CHUNK_SIZE_X) % CHUNK_SIZE_X;
        int nz = (lz % CHUNK_SIZE_Z + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;
        return n->GetBlock(nx, ly, nz);
    }
    return BlockType::Air;
}

unsigned char Chunk::GetLightSafe(int lx, int ly, int lz, Chunk** neighbors) {
    if (ly < 0 || ly >= CHUNK_SIZE_Y) return 15;

    int cx = 0, cz = 0;
    if (lx < 0) cx = -1; else if (lx >= CHUNK_SIZE_X) cx = 1;
    if (lz < 0) cz = -1; else if (lz >= CHUNK_SIZE_Z) cz = 1;

    if (cx == 0 && cz == 0) return lightMap[lx + CHUNK_SIZE_X * (lz + CHUNK_SIZE_Z * ly)];

    Chunk* n = neighbors[(cx + 1) + (cz + 1) * 3];
    if (n) {
        int nx = (lx % CHUNK_SIZE_X + CHUNK_SIZE_X) % CHUNK_SIZE_X;
        int nz = (lz % CHUNK_SIZE_Z + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;
        return n->GetLight(nx, ly, nz);
    }
    return 15;
}

int Chunk::GetLightEmission(BlockType t) {
    switch (t) {
    case BlockType::Torch:      return 14;
    case BlockType::Lava:
    case BlockType::LavaSource: return 12;
    default:                    return 0;
    }
}

bool Chunk::IsLightPassable(BlockType t) {
    return t == BlockType::Air || t == BlockType::Glass || t == BlockType::OakSapling ||
        t == BlockType::Fern || t == BlockType::Reed || t == BlockType::Rose ||
        t == BlockType::BerryBush || t == BlockType::BerryBushRipe ||
        t == BlockType::Cattail || t == BlockType::LilyPad ||
            t == BlockType::CranberryBush || t == BlockType::Toadstool ||
            t == BlockType::Clover || t == BlockType::DuneGrass ||
        t == BlockType::Dandelion || t == BlockType::TallGrass || t == BlockType::Water ||
        t == BlockType::Torch ||
        t == BlockType::DeadBush || t == BlockType::BrownMushroom || t == BlockType::RedMushroom ||
        t == BlockType::OakLeaves || t == BlockType::SpruceLeaves || t == BlockType::BirchLeaves ||
        t == BlockType::AcaciaLeaves || t == BlockType::JungleLeaves;
}


BlockType Chunk::GetBlock(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE_X || z < 0 || z >= CHUNK_SIZE_Z || y < 0 || y >= CHUNK_SIZE_Y) return BlockType::Air;
    return blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)];
}

void Chunk::SetBlock(int x, int y, int z, int type) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (x >= 0 && x < CHUNK_SIZE_X && z >= 0 && z < CHUNK_SIZE_Z && y >= 0 && y < CHUNK_SIZE_Y) {
        blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = (BlockType)type;
        isModified = true;
        lightPasses = 0;
        if (type != 0 && y > maxY - 5) {
            maxY = y + 10;
            if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;
        }
    }
}

void Chunk::SetBlockRaw(int x, int y, int z, int type) {
    blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = (BlockType)type;
    isModified = true;
    if (type != 0 && y > maxY - 5) {
        maxY = y + 10;
        if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;
    }
}

