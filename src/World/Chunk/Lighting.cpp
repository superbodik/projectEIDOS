#include "../Chunk.h"
#include <algorithm>
#include <queue>

bool Chunk::CalculateBasicSunlight(Chunk** neighbors) {
    const int N = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    std::vector<unsigned char> tSky(N, 0), tBlk(N, 0);
    std::queue<int> lightQ, blockQ;

    auto isLightPassable = [](BlockType t) { return IsLightPassable(t); };

    auto dimmer = [](BlockType t) -> unsigned char {
        if (t == BlockType::Water || t == BlockType::OakLeaves || t == BlockType::SpruceLeaves ||
            t == BlockType::BirchLeaves || t == BlockType::AcaciaLeaves || t == BlockType::JungleLeaves)
            return 2;
        return 1;
        };

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            unsigned char currentLight = 15;
            for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType t = blocks[idx];

                if (!isLightPassable(t)) currentLight = 0;
                else if (dimmer(t) == 2) {
                    if (currentLight > 2) currentLight -= 2; else currentLight = 0;
                }

                tSky[idx] = currentLight;
                if (currentLight > 0 && y <= maxY) {
                    lightQ.push(x); lightQ.push(y); lightQ.push(z);
                }

                int emit = GetLightEmission(t);
                if (emit > 0 && y <= maxY) {
                    tBlk[idx] = (unsigned char)emit;
                    blockQ.push(x); blockQ.push(y); blockQ.push(z);
                }
            }
        }
    }

    Chunk* nXneg = neighbors[0 + 1 * 3]; Chunk* nXpos = neighbors[2 + 1 * 3];
    Chunk* nZneg = neighbors[1 + 0 * 3]; Chunk* nZpos = neighbors[1 + 2 * 3];

    auto borderLight = [&](Chunk* n, int nx, int ny, int nz, bool wantSky) -> int {
        if (!n || n->dirty) return wantSky ? 15 : 0;
        BlockType t = n->GetBlock(nx, ny, nz);
        if (!IsLightPassable(t)) return 0;
        unsigned char packed = n->GetLight(nx, ny, nz);
        return wantSky ? SkyLightOf(packed) : BlockLightOf(packed);
        };

    for (int y = 0; y <= maxY; y++) {
        for (int x = 0; x < CHUNK_SIZE_X; x++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                if (x != 0 && x != CHUNK_SIZE_X - 1 && z != 0 && z != CHUNK_SIZE_Z - 1) continue;
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType t = blocks[idx];
                if (!isLightPassable(t)) continue;

                unsigned char drop = dimmer(t);

                for (int pass = 0; pass < 2; pass++) {
                    bool wantSky = (pass == 0);
                    int best = 0;
                    if (x == 0) best = std::max(best, borderLight(nXneg, 15, y, z, wantSky));
                    if (x == CHUNK_SIZE_X - 1) best = std::max(best, borderLight(nXpos, 0, y, z, wantSky));
                    if (z == 0) best = std::max(best, borderLight(nZneg, x, y, 15, wantSky));
                    if (z == CHUNK_SIZE_Z - 1) best = std::max(best, borderLight(nZpos, x, y, 0, wantSky));

                    if (best <= drop) continue;
                    unsigned char newL = (unsigned char)(best - drop);
                    std::vector<unsigned char>& target = wantSky ? tSky : tBlk;
                    if (newL > target[idx]) {
                        target[idx] = newL;
                        std::queue<int>& q = wantSky ? lightQ : blockQ;
                        q.push(x); q.push(y); q.push(z);
                    }
                }
            }
        }
    }

    const int dirs[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };

    auto flood = [&](std::queue<int>& q, std::vector<unsigned char>& field) {
        while (!q.empty()) {
            int qx = q.front(); q.pop();
            int qy = q.front(); q.pop();
            int qz = q.front(); q.pop();
            unsigned char lightLvl = field[qx + CHUNK_SIZE_X * (qz + CHUNK_SIZE_Z * qy)];

            for (int i = 0; i < 6; i++) {
                int nx = qx + dirs[i][0], ny = qy + dirs[i][1], nz = qz + dirs[i][2];
                if (nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny > maxY || nz < 0 || nz >= CHUNK_SIZE_Z) continue;
                int nIdx = nx + CHUNK_SIZE_X * (nz + CHUNK_SIZE_Z * ny);
                BlockType nt = blocks[nIdx];
                if (!isLightPassable(nt)) continue;

                unsigned char drop = dimmer(nt);
                if (lightLvl <= drop) continue;
                unsigned char newLight = lightLvl - drop;
                if (newLight > field[nIdx]) {
                    field[nIdx] = newLight;
                    q.push(nx); q.push(ny); q.push(nz);
                }
            }
        }
        };

    flood(lightQ, tSky);
    flood(blockQ, tBlk);

    std::vector<unsigned char> packed(N);
    for (int i = 0; i < N; i++)
        packed[i] = (unsigned char)((tSky[i] << 4) | (tBlk[i] & 0x0F));

    bool lightChanged = false;
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        if (memcmp(lightMap, packed.data(), packed.size()) != 0) {
            lightChanged = true;
            memcpy(lightMap, packed.data(), packed.size());
        }
    }
    return lightChanged;
}

