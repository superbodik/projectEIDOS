#include "Chunk.h"
#include <fstream>
#include <iostream>
#include <raymath.h>
#include <algorithm>
#include <queue>
#include <cstring> 

Texture2D Chunk::atlasTexture = { 0 };

void Chunk::LoadAtlas() {
    if (atlasTexture.id != 0) return;
    Image atlasImg = GenImageColor(64, 64, MAGENTA);
    auto DrawBlock = [&](int col, int row, Color base, int noiseAmount) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                int n = GetRandomValue(-noiseAmount, noiseAmount);
                unsigned char r = (unsigned char)Clamp((float)(base.r + n), 0.0f, 255.0f);
                unsigned char g = (unsigned char)Clamp((float)(base.g + n), 0.0f, 255.0f);
                unsigned char b = (unsigned char)Clamp((float)(base.b + n), 0.0f, 255.0f);
                ImageDrawPixel(&atlasImg, col * 16 + x, row * 16 + y, { r, g, b, 255 });
            }
        }
        };
    DrawBlock(0, 0, { 34, 139, 34, 255 }, 20); DrawBlock(1, 0, { 237, 201, 175, 255 }, 15);
    DrawBlock(2, 0, { 128, 128, 128, 255 }, 20); DrawBlock(3, 0, { 101, 67, 33, 255 }, 15);
    DrawBlock(0, 1, { 80, 50, 20, 255 }, 15); DrawBlock(1, 1, { 220, 220, 220, 255 }, 5);
    DrawBlock(2, 1, { 47, 79, 79, 255 }, 15); DrawBlock(3, 1, { 34, 100, 34, 255 }, 20);
    DrawBlock(0, 2, { 20, 60, 20, 255 }, 20); DrawBlock(1, 2, { 100, 200, 100, 255 }, 20);
    DrawBlock(2, 2, { 0, 120, 255, 200 }, 10); DrawBlock(3, 2, { 101, 67, 33, 255 }, 15);
    ImageDrawRectangle(&atlasImg, 3 * 16, 2 * 16, 16, 4, { 34, 139, 34, 255 });
    DrawBlock(0, 3, { 255, 0, 255, 255 }, 0); DrawBlock(1, 3, { 100, 100, 100, 255 }, 30);
    DrawBlock(2, 3, { 50, 50, 50, 255 }, 10); DrawBlock(3, 3, { 160, 100, 50, 255 }, 10);
    atlasTexture = LoadTextureFromImage(atlasImg);
    SetTextureFilter(atlasTexture, TEXTURE_FILTER_POINT);
    UnloadImage(atlasImg);
}

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    blocks = new BlockType[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    lightMap = new unsigned char[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    mesh = { 0 }; model = { 0 };
}

Chunk::~Chunk() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (hasMesh) UnloadModel(model);
    delete[] blocks;
    delete[] lightMap;
}

void Chunk::GenerateTerrain(WorldGenerator& gen) {
    maxY = 0;
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int wx = chunkX * CHUNK_SIZE_X + x;
            int wz = chunkZ * CHUNK_SIZE_Z + z;

            int currentH = gen.GetHeight(wx, wz);
            if (currentH > maxY) maxY = currentH;

            for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = gen.GetBlock(wx, y, wz);
            }
        }
    }
    maxY += 30;
    if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;
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

bool Chunk::CalculateBasicSunlight(Chunk** neighbors) {
    std::vector<unsigned char> tLight(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z, 0);
    std::queue<int> lightQ;

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            unsigned char currentLight = 15;
            for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType t = blocks[idx];

                bool isTransp = (t == BlockType::Air || t == BlockType::Glass || t == BlockType::Rose || t == BlockType::Dandelion || t == BlockType::TallGrass || t == BlockType::Water);

                if (!isTransp) currentLight = 0;
                else if (t == BlockType::Water) {
                    if (currentLight > 2) currentLight -= 2; else currentLight = 0;
                }

                tLight[idx] = currentLight;
                if (currentLight > 0 && y <= maxY) {
                    lightQ.push(x); lightQ.push(y); lightQ.push(z);
                }
            }
        }
    }

    Chunk* nXneg = neighbors[0 + 1 * 3];
    Chunk* nXpos = neighbors[2 + 1 * 3];
    Chunk* nZneg = neighbors[1 + 0 * 3];
    Chunk* nZpos = neighbors[1 + 2 * 3];

    auto getLightIfTransp = [](Chunk* n, int nx, int ny, int nz) -> int {
        if (!n || n->dirty) return 0;
        BlockType t = n->GetBlock(nx, ny, nz);
        bool transp = (t == BlockType::Air || t == BlockType::Glass || t == BlockType::Water || t == BlockType::Rose || t == BlockType::Dandelion || t == BlockType::TallGrass);
        if (!transp) return 0;
        return n->GetLight(nx, ny, nz);
        };

    for (int y = 0; y <= maxY; y++) {
        for (int x = 0; x < CHUNK_SIZE_X; x++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                if (x == 0 || x == CHUNK_SIZE_X - 1 || z == 0 || z == CHUNK_SIZE_Z - 1) {
                    int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                    BlockType t = blocks[idx];
                    bool isTransp = (t == BlockType::Air || t == BlockType::Glass || t == BlockType::Water || t == BlockType::Rose || t == BlockType::Dandelion || t == BlockType::TallGrass);

                    if (isTransp) {
                        int maxNeighborLight = 0;
                        if (x == 0) maxNeighborLight = std::max(maxNeighborLight, getLightIfTransp(nXneg, 15, y, z));
                        if (x == CHUNK_SIZE_X - 1) maxNeighborLight = std::max(maxNeighborLight, getLightIfTransp(nXpos, 0, y, z));
                        if (z == 0) maxNeighborLight = std::max(maxNeighborLight, getLightIfTransp(nZneg, x, y, 15));
                        if (z == CHUNK_SIZE_Z - 1) maxNeighborLight = std::max(maxNeighborLight, getLightIfTransp(nZpos, x, y, 0));

                        unsigned char drop = (t == BlockType::Water) ? 2 : 1;
                        if (maxNeighborLight > drop) {
                            unsigned char newL = maxNeighborLight - drop;
                            if (newL > tLight[idx]) {
                                tLight[idx] = newL;
                                lightQ.push(x); lightQ.push(y); lightQ.push(z);
                            }
                        }
                    }
                }
            }
        }
    }

    int dirs[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };

    while (!lightQ.empty()) {
        int qx = lightQ.front(); lightQ.pop();
        int qy = lightQ.front(); lightQ.pop();
        int qz = lightQ.front(); lightQ.pop();
        int idx = qx + CHUNK_SIZE_X * (qz + CHUNK_SIZE_Z * qy);
        unsigned char lightLvl = tLight[idx];

        for (int i = 0; i < 6; i++) {
            int nx = qx + dirs[i][0]; int ny = qy + dirs[i][1]; int nz = qz + dirs[i][2];
            if (nx >= 0 && nx < CHUNK_SIZE_X && ny >= 0 && ny <= maxY && nz >= 0 && nz < CHUNK_SIZE_Z) {
                int nIdx = nx + CHUNK_SIZE_X * (nz + CHUNK_SIZE_Z * ny);
                BlockType nt = blocks[nIdx];
                bool nTransp = (nt == BlockType::Air || nt == BlockType::Glass || nt == BlockType::Water || nt == BlockType::Rose || nt == BlockType::Dandelion || nt == BlockType::TallGrass);

                if (nTransp) {
                    unsigned char drop = (nt == BlockType::Water) ? 2 : 1;
                    if (lightLvl > drop) {
                        unsigned char newLight = lightLvl - drop;
                        if (newLight > tLight[nIdx]) {
                            tLight[nIdx] = newLight;
                            lightQ.push(nx); lightQ.push(ny); lightQ.push(nz);
                        }
                    }
                }
            }
        }
    }

    bool lightChanged = false;
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        if (memcmp(lightMap, tLight.data(), tLight.size()) != 0) {
            lightChanged = true;
            memcpy(lightMap, tLight.data(), tLight.size());
        }
    }
    return lightChanged;
}

bool Chunk::IsSolid(int x, int y, int z, Chunk** neighbors) {
    BlockType type = GetBlockSafe(x, y, z, neighbors);
    return type != BlockType::Air && type != BlockType::Water && type != BlockType::Glass && type != BlockType::Rose && type != BlockType::Dandelion && type != BlockType::TallGrass;
}

void Chunk::GetTextureUV(BlockType type, int faceDir, float& u, float& v) {
    int col = 0; int row = 0;
    switch (type) {
    case BlockType::Grass: if (faceDir == 0) { col = 0;row = 0; }
                         else if (faceDir == 1) { col = 3;row = 0; }
                         else { col = 3;row = 2; } break;
    case BlockType::Dirt: col = 3; row = 0; break;
    case BlockType::Sand: case BlockType::RedSand: col = 1; row = 0; break;
    case BlockType::Stone: case BlockType::Granite: case BlockType::Diorite: case BlockType::Andesite: col = 2; row = 0; break;
    case BlockType::Bedrock: col = 2; row = 3; break;
    case BlockType::Cobblestone: col = 1; row = 3; break;
    case BlockType::OakLog: if (faceDir == 0 || faceDir == 1) { col = 3;row = 3; }
                          else { col = 0;row = 1; } break;
    case BlockType::SpruceLog: if (faceDir == 0 || faceDir == 1) { col = 3;row = 3; }
                             else { col = 2;row = 1; } break;
    case BlockType::BirchLog: if (faceDir == 0 || faceDir == 1) { col = 3;row = 3; }
                            else { col = 1;row = 1; } break;
    case BlockType::OakLeaves: col = 3; row = 1; break;
    case BlockType::SpruceLeaves: col = 0; row = 2; break;
    case BlockType::BirchLeaves: col = 1; row = 2; break;
    case BlockType::Gravel: col = 1; row = 3; break;
    case BlockType::Water: col = 2; row = 2; break;
    case BlockType::Snow: col = 1; row = 1; break;
    default: col = 0; row = 3; break;
    }
    u = col * 0.25f; v = row * 0.25f;
}

bool Chunk::BuildMeshCPU(WorldGenerator& gen, Chunk** neighbors) {
    (void)gen;
    bool lightChanged = CalculateBasicSunlight(neighbors);

    std::vector<float> tVerts;
    std::vector<float> tTex;
    std::vector<unsigned char> tCols;
    int tCount = 0;

    tVerts.reserve(15000);
    tTex.reserve(10000);
    tCols.reserve(20000);

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y <= maxY; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType type = blocks[idx];
                if (type == BlockType::Air) continue;

                if (!IsSolid(x, y + 1, z, neighbors)) AddFace(x, y, z, 0, type, neighbors, tVerts, tTex, tCols, tCount);
                if (!IsSolid(x, y - 1, z, neighbors)) AddFace(x, y, z, 1, type, neighbors, tVerts, tTex, tCols, tCount);
                if (!IsSolid(x, y, z + 1, neighbors)) AddFace(x, y, z, 2, type, neighbors, tVerts, tTex, tCols, tCount);
                if (!IsSolid(x, y, z - 1, neighbors)) AddFace(x, y, z, 3, type, neighbors, tVerts, tTex, tCols, tCount);
                if (!IsSolid(x + 1, y, z, neighbors)) AddFace(x, y, z, 4, type, neighbors, tVerts, tTex, tCols, tCount);
                if (!IsSolid(x - 1, y, z, neighbors)) AddFace(x, y, z, 5, type, neighbors, tVerts, tTex, tCols, tCount);
            }
        }
    }

    std::lock_guard<std::mutex> lock(chunkMutex);
    std::swap(vertices, tVerts);
    std::swap(texcoords, tTex);
    std::swap(colors, tCols);
    vertexCount = tCount;
    state = 2;

    return lightChanged;
}

void Chunk::AddFace(int x, int y, int z, int faceDir, BlockType type, Chunk** neighbors,
    std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<unsigned char>& tCols, int& tCount) {
    float texU, texV;
    GetTextureUV(type, faceDir, texU, texV);
    float step = 0.25f; float offset = 0.005f;
    float uMin = texU + offset; float uMax = texU + step - offset;
    float vMin = texV + offset; float vMax = texV + step - offset;

    int lx = 0, ly = 0, lz = 0;
    if (faceDir == 0) ly = 1; else if (faceDir == 1) ly = -1;
    else if (faceDir == 2) lz = 1; else if (faceDir == 3) lz = -1;
    else if (faceDir == 4) lx = 1; else if (faceDir == 5) lx = -1;

    float faceShading = 1.0f;
    if (faceDir == 1) faceShading = 0.5f;
    else if (faceDir == 2 || faceDir == 3) faceShading = 0.85f;
    else if (faceDir == 4 || faceDir == 5) faceShading = 0.7f;

    auto getS = [&](int dx, int dy, int dz) { return IsSolid(x + dx, y + dy, z + dz, neighbors); };
    auto getL = [&](int dx, int dy, int dz) { return (float)GetLightSafe(x + dx, y + dy, z + dz, neighbors); };

    auto calcLightAO = [&](int side1X, int side1Y, int side1Z,
        int side2X, int side2Y, int side2Z,
        int cornX, int cornY, int cornZ,
        float& lOut, int& aoOut) {
            bool s1 = getS(side1X, side1Y, side1Z);
            bool s2 = getS(side2X, side2Y, side2Z);
            bool c = getS(cornX, cornY, cornZ);

            if (s1 && s2) aoOut = 0;
            else aoOut = 3 - (s1 + s2 + c);

            float l0 = getL(lx, ly, lz);
            float l1 = getL(side1X, side1Y, side1Z);
            float l2 = getL(side2X, side2Y, side2Z);
            float l3 = getL(cornX, cornY, cornZ);

            if (s1) l1 = l0;
            if (s2) l2 = l0;
            if (c)  l3 = l0;

            lOut = (l0 + l1 + l2 + l3) / 4.0f;
        };

    float vL[4]; int ao[4];

    if (faceDir == 0) {
        calcLightAO(-1, 1, 0, 0, 1, -1, -1, 1, -1, vL[0], ao[0]);
        calcLightAO(-1, 1, 0, 0, 1, 1, -1, 1, 1, vL[1], ao[1]);
        calcLightAO(1, 1, 0, 0, 1, 1, 1, 1, 1, vL[2], ao[2]);
        calcLightAO(1, 1, 0, 0, 1, -1, 1, 1, -1, vL[3], ao[3]);
    }
    else if (faceDir == 1) {
        calcLightAO(-1, -1, 0, 0, -1, 1, -1, -1, 1, vL[0], ao[0]);
        calcLightAO(-1, -1, 0, 0, -1, -1, -1, -1, -1, vL[1], ao[1]);
        calcLightAO(1, -1, 0, 0, -1, -1, 1, -1, -1, vL[2], ao[2]);
        calcLightAO(1, -1, 0, 0, -1, 1, 1, -1, 1, vL[3], ao[3]);
    }
    else if (faceDir == 2) {
        calcLightAO(-1, 0, 1, 0, 1, 1, -1, 1, 1, vL[0], ao[0]);
        calcLightAO(-1, 0, 1, 0, -1, 1, -1, -1, 1, vL[1], ao[1]);
        calcLightAO(1, 0, 1, 0, -1, 1, 1, -1, 1, vL[2], ao[2]);
        calcLightAO(1, 0, 1, 0, 1, 1, 1, 1, 1, vL[3], ao[3]);
    }
    else if (faceDir == 3) {
        calcLightAO(1, 0, -1, 0, 1, -1, 1, 1, -1, vL[0], ao[0]);
        calcLightAO(1, 0, -1, 0, -1, -1, 1, -1, -1, vL[1], ao[1]);
        calcLightAO(-1, 0, -1, 0, -1, -1, -1, -1, -1, vL[2], ao[2]);
        calcLightAO(-1, 0, -1, 0, 1, -1, -1, 1, -1, vL[3], ao[3]);
    }
    else if (faceDir == 4) {
        calcLightAO(1, 0, 1, 1, 1, 0, 1, 1, 1, vL[0], ao[0]);
        calcLightAO(1, 0, 1, 1, -1, 0, 1, -1, 1, vL[1], ao[1]);
        calcLightAO(1, 0, -1, 1, -1, 0, 1, -1, -1, vL[2], ao[2]);
        calcLightAO(1, 0, -1, 1, 1, 0, 1, 1, -1, vL[3], ao[3]);
    }
    else if (faceDir == 5) {
        calcLightAO(-1, 0, -1, -1, 1, 0, -1, 1, -1, vL[0], ao[0]);
        calcLightAO(-1, 0, -1, -1, -1, 0, -1, -1, -1, vL[1], ao[1]);
        calcLightAO(-1, 0, 1, -1, -1, 0, -1, -1, 1, vL[2], ao[2]);
        calcLightAO(-1, 0, 1, -1, 1, 0, -1, 1, 1, vL[3], ao[3]);
    }

    unsigned char cAO[4];
    for (int i = 0; i < 4; i++) {
        float aoBase = 120.0f + (float)ao[i] * 45.0f;
        float finalLightMult = std::max(0.12f, vL[i] / 15.0f);
        cAO[i] = (unsigned char)(aoBase * finalLightMult * faceShading);
    }

    float bx = (float)x; float by = (float)y; float bz = (float)z;
    float v[4][3];
    if (faceDir == 0) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx;v[1][1] = by + 1;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by + 1;v[2][2] = bz + 1;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 1) {
        v[0][0] = bx;   v[0][1] = by; v[0][2] = bz;
        v[1][0] = bx + 1; v[1][1] = by; v[1][2] = bz;
        v[2][0] = bx + 1; v[2][1] = by; v[2][2] = bz + 1;
        v[3][0] = bx;   v[3][1] = by; v[3][2] = bz + 1;
    }
    else if (faceDir == 2) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz + 1;v[1][0] = bx;v[1][1] = by;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by;v[2][2] = bz + 1;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz + 1; }
    else if (faceDir == 3) { v[0][0] = bx + 1;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx + 1;v[1][1] = by;v[1][2] = bz;v[2][0] = bx;v[2][1] = by;v[2][2] = bz;v[3][0] = bx;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 4) { v[0][0] = bx + 1;v[0][1] = by + 1;v[0][2] = bz + 1;v[1][0] = bx + 1;v[1][1] = by;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by;v[2][2] = bz;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 5) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx;v[1][1] = by;v[1][2] = bz;v[2][0] = bx;v[2][1] = by;v[2][2] = bz + 1;v[3][0] = bx;v[3][1] = by + 1;v[3][2] = bz + 1; }

    int tri1[3] = { 0, 1, 2 };
    for (int i : tri1) {
        tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
        tCols.push_back(cAO[i]); tCols.push_back(cAO[i]); tCols.push_back(cAO[i]); tCols.push_back(255);
    }
    tTex.push_back(uMin); tTex.push_back(vMin);
    tTex.push_back(uMin); tTex.push_back(vMax);
    tTex.push_back(uMax); tTex.push_back(vMax);

    int tri2[3] = { 0, 2, 3 };
    for (int i : tri2) {
        tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
        tCols.push_back(cAO[i]); tCols.push_back(cAO[i]); tCols.push_back(cAO[i]); tCols.push_back(255);
    }
    tTex.push_back(uMin); tTex.push_back(vMin);
    tTex.push_back(uMax); tTex.push_back(vMax);
    tTex.push_back(uMax); tTex.push_back(vMin);

    tCount += 6;
}

void Chunk::UploadMeshGPU() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (vertexCount == 0) { state = 3; if (hasMesh) { UnloadModel(model); hasMesh = false; } return; }

    Mesh newMesh = { 0 };
    newMesh.vertexCount = vertexCount; newMesh.triangleCount = vertexCount / 3;
    newMesh.vertices = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
    newMesh.texcoords = (float*)MemAlloc(vertexCount * 2 * sizeof(float));
    newMesh.colors = (unsigned char*)MemAlloc(vertexCount * 4 * sizeof(unsigned char));

    memcpy(newMesh.vertices, vertices.data(), vertices.size() * sizeof(float));
    memcpy(newMesh.texcoords, texcoords.data(), texcoords.size() * sizeof(float));
    memcpy(newMesh.colors, colors.data(), colors.size() * sizeof(unsigned char));

    UploadMesh(&newMesh, false);
    Model newModel = LoadModelFromMesh(newMesh);
    newModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = atlasTexture;

    if (hasMesh) UnloadModel(model);
    model = newModel; hasMesh = true; state = 3;

    vertices.clear(); vertices.shrink_to_fit();
    texcoords.clear(); texcoords.shrink_to_fit();
    colors.clear(); colors.shrink_to_fit();
}

void Chunk::Draw() {
    if (hasMesh) {
        Vector3 pos = { (float)chunkX * 16, 0, (float)chunkZ * 16 };
        DrawModel(model, pos, 1.0f, WHITE);
    }
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

bool Chunk::SaveToFile(const std::string& worldPath) {
    if (!isModified) return true;

    std::string file = worldPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".bin";
    std::ofstream out(file, std::ios::binary);
    if (!out) return false;

    out.write((char*)blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));
    out.close();

    isModified = false;
    return true;
}

bool Chunk::LoadFromFile(const std::string& worldPath) {
    std::string file = worldPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".bin";
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;

    in.read((char*)blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));
    in.close();

    maxY = 0;
    for (int y = 0; y < CHUNK_SIZE_Y; y++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                if (blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] != BlockType::Air) {
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    maxY += 30;
    if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;

    state = 1;
    isModified = false;
    return true;
}