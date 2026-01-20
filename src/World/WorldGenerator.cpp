#include "WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

const float POLE_DISTANCE = 20000.0f;
const float NOISE_WOBBLE = 1500.0f;

static const BlockType IGNEOUS_INTRUSIVE[] = { BlockType::Granite, BlockType::Diorite, BlockType::Gabbro };
static const BlockType IGNEOUS_EXTRUSIVE[] = { BlockType::Rhyolite, BlockType::Basalt, BlockType::Andesite, BlockType::Dacite };
static const BlockType SEDIMENTARY[] = { BlockType::Shale, BlockType::Claystone, BlockType::Limestone, BlockType::Conglomerate, BlockType::Dolomite, BlockType::Chert, BlockType::Chalk };
static const BlockType METAMORPHIC[] = { BlockType::Quartzite, BlockType::Slate, BlockType::Phyllite, BlockType::Schist, BlockType::Gneiss, BlockType::Marble };

WorldGenerator::WorldGenerator(int _seed) : seed(_seed) {}

static float QuinticFade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static float Smoothstep(float edge0, float edge1, float x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
}

float Noise3D_Fake(WorldGenerator* gen, int x, int y, int z, float scale) {
    float yOffset = (float)y * scale * 0.5f;
    return gen->Noise2D((float)x * scale + yOffset, (float)z * scale + yOffset);
}

float WorldGenerator::PseudoRandom(int x, int y) {
    int n = x + y * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float WorldGenerator::RandomGradient(int ix, int iz) {
    return PseudoRandom(ix, iz);
}

float WorldGenerator::Noise2D(float x, float z) {
    int x0 = (int)floor(x);
    int z0 = (int)floor(z);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float sx = x - (float)x0;
    float sz = z - (float)z0;

    float n0 = RandomGradient(x0, z0);
    float n1 = RandomGradient(x1, z0);
    float n2 = RandomGradient(x0, z1);
    float n3 = RandomGradient(x1, z1);

    float u = QuinticFade(sx);
    float v = QuinticFade(sz);

    float ix0 = Lerp(n0, n1, u);
    float ix1 = Lerp(n2, n3, u);

    return Lerp(ix0, ix1, v);
}

float WorldGenerator::FBM(float x, float z, int octaves, float persistence, float scale) {
    float total = 0;
    float frequency = scale;
    float amplitude = 1;
    float maxValue = 0;
    for (int i = 0; i < octaves; i++) {
        total += Noise2D(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2;
    }
    return total / maxValue;
}

BlockType WorldGenerator::GetRockLayer(int x, int y, int z) {
    (void)y;
    float geoRegion = Noise2D((float)x * 0.002f + seed, (float)z * 0.002f + seed);
    float localRock = Noise2D((float)x * 0.01f, (float)z * 0.01f);

    if (geoRegion < -0.3f) return SEDIMENTARY[(int)(std::abs(localRock) * 7) % 7];
    else if (geoRegion < 0.2f) return METAMORPHIC[(int)(std::abs(localRock) * 6) % 6];
    else if (geoRegion < 0.6f) return IGNEOUS_INTRUSIVE[(int)(std::abs(localRock) * 3) % 3];
    else return IGNEOUS_EXTRUSIVE[(int)(std::abs(localRock) * 4) % 4];
}

BlockType WorldGenerator::GetOre(int x, int y, int z, BlockType rockType) {
    bool isSedimentary = (rockType >= BlockType::Shale && rockType <= BlockType::Chalk);
    bool isIgneous = (rockType >= BlockType::Granite && rockType <= BlockType::Dacite);

    if (isSedimentary) {
        if (Noise3D_Fake(this, x, y, z, 0.04f) > 0.65f) return BlockType::BituminousCoal;
    }

    float ironNoise = Noise3D_Fake(this, x + 100, y, z + 100, 0.05f);
    if (ironNoise > 0.75f) {
        if (isSedimentary) return BlockType::Limonite;
        if (isIgneous && ironNoise > 0.85f) return BlockType::Magnetite;
        return BlockType::Hematite;
    }

    float copperNoise = Noise3D_Fake(this, x + 500, y, z + 500, 0.06f);
    if (copperNoise > 0.80f) {
        if (rockType == BlockType::Limestone || rockType == BlockType::Marble || rockType == BlockType::Chalk)
            return BlockType::Malachite;
        if (isIgneous) return BlockType::NativeCopper;
        return BlockType::Tetrahedrite;
    }

    if (y < 40 && isIgneous) {
        if (Noise3D_Fake(this, x + 999, y, z + 999, 0.08f) > 0.88f) return BlockType::NativeGold;
    }

    return BlockType::Air;
}

float WorldGenerator::GetTemperature(int x, int z) {
    float wobble = Noise2D((float)x * 0.001f, (float)z * 0.001f) * NOISE_WOBBLE;
    float adjustedZ = std::abs((float)z + wobble);
    float latitude = std::clamp(adjustedZ / POLE_DISTANCE, 0.0f, 1.0f);
    float baseTemp = 1.0f - latitude;
    float localVariation = Noise2D((float)x * 0.005f, (float)z * 0.005f) * 0.1f;
    return std::clamp(baseTemp + localVariation, 0.0f, 1.0f);
}

float WorldGenerator::GetHumidity(int x, int z) {
    float rawNoise = Noise2D((float)x * 0.0008f + 500, (float)z * 0.0008f + 500);
    return (rawNoise + 1.0f) / 2.0f;
}

BlockType WorldGenerator::GetSurfaceBlock(int x, int z, int height, float temp, float hum) {
    (void)x; (void)z;

    if (height < SEA_LEVEL) return BlockType::Water;
    if (height > MOUNTAIN_LEVEL - 10) return BlockType::Snow;
    if (height > MOUNTAIN_LEVEL - 20 && temp < 0.5f) return BlockType::Snow;

    if (temp < 0.25f) return BlockType::Snow;
    else if (temp < 0.5f) {
        if (hum < 0.2f) return BlockType::Gravel;
        return BlockType::Grass;
    }
    else if (temp < 0.75f) {
        if (hum < 0.3f) return BlockType::Sand;
        return BlockType::Grass;
    }
    else {
        if (hum < 0.4f) return BlockType::Sand;
        if (hum < 0.6f) return BlockType::Grass;
        return BlockType::Grass;
    }
}

BlockType WorldGenerator::GetBlock(int x, int y, int z) {
    int height = GetHeight(x, z);
    float river = GetRiverFactor(x, z);

    if (y <= SEA_LEVEL && river > 0.85f) return BlockType::Water;
    if (y > height) {
        if (y <= SEA_LEVEL) return BlockType::Water;
        return BlockType::Air;
    }

    float temp = GetTemperature(x, z);
    float hum = GetHumidity(x, z);

    if (y == height) {
        if (river > 0.80f && river <= 0.85f && y <= SEA_LEVEL + 2) return BlockType::Sand;
        if (river > 0.80f && river <= 0.85f && hum > 0.7f) return BlockType::Clay;
        return GetSurfaceBlock(x, z, height, temp, hum);
    }

    if (y > height - 4) {
        BlockType surf = GetSurfaceBlock(x, z, height, temp, hum);
        if (surf == BlockType::Grass) return BlockType::Dirt;
        if (surf == BlockType::Sand) return BlockType::Sand;
        return BlockType::Dirt;
    }

    if (y <= 0) return BlockType::Bedrock;
    if (y < 3 && Noise3D_Fake(this, x, y, z, 0.5f) > 0.0f) return BlockType::Bedrock;

    BlockType rock = GetRockLayer(x, y, z);
    BlockType ore = GetOre(x, y, z, rock);

    if (ore != BlockType::Air) return ore;
    return rock;
}

int WorldGenerator::GetHeight(int x, int z) {
    float continent = GetContinentalness(x, z);
    float h = (float)SEA_LEVEL;

    if (continent < 0) {
        h += continent * 40.0f;
    }
    else {
        h += 5.0f;
        float lowFreq = Noise2D((float)x * 0.003f, (float)z * 0.003f);
        float highFreq = Noise2D((float)x * 0.02f, (float)z * 0.02f);
        h += lowFreq * 30.0f + highFreq * 5.0f;

        float mountainBlend = Smoothstep(0.4f, 0.8f, continent);
        if (mountainBlend > 0.01f) {
            float peaks = GetPeaks(x, z);
            peaks = std::pow(peaks, 3.0f);
            h += peaks * 300.0f * mountainBlend;
        }
    }

    float river = GetRiverFactor(x, z);
    if (river > 0.85f) {
        float depth = (river - 0.85f) * 100.0f;
        h -= depth;
        if (h < SEA_LEVEL - 10) h = SEA_LEVEL - 10;
    }
    else if (river > 0.80f) {
        float slope = (river - 0.80f) * 50.0f;
        h -= slope;
    }

    if (h < 5.0f) h = 5.0f;
    return (int)h;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetTreeAt(int x, int y, int z) {
    std::vector<TreeBlock> tree;
    float n = Noise2D((float)x * 0.5f, (float)z * 0.5f);
    int height = 5 + (int)(std::abs(n) * 4);

    for (int i = 0; i < height; i++) tree.push_back({ x, y + i, z, BlockType::OakLog });

    int leavesStart = height - 2; int leavesEnd = height + 1;
    for (int ly = leavesStart; ly <= leavesEnd; ly++) {
        int radius = (ly == leavesEnd) ? 1 : 2;
        if (ly == leavesEnd - 1) radius = 2;
        for (int lx = -radius; lx <= radius; lx++) {
            for (int lz = -radius; lz <= radius; lz++) {
                if (std::abs(lx) + std::abs(lz) > radius + 1) continue;
                if (lx == 0 && lz == 0 && ly < height) continue;
                tree.push_back({ x + lx, y + ly, z + lz, BlockType::OakLeaves });
            }
        }
    }
    return tree;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetSpruceAt(int x, int y, int z) {
    std::vector<TreeBlock> tree;
    float n = Noise2D((float)x * 0.5f, (float)z * 0.5f);
    int height = 8 + (int)(std::abs(n) * 5);

    for (int i = 0; i < height; i++) tree.push_back({ x, y + i, z, BlockType::SpruceLog });

    for (int ly = 2; ly < height; ly++) {
        int radius = 2;
        if (ly > height / 2) radius = 1;
        if (ly >= height - 1) radius = 0;
        for (int lx = -radius; lx <= radius; lx++) {
            for (int lz = -radius; lz <= radius; lz++) {
                if (std::abs(lx) == radius && std::abs(lz) == radius && radius > 0) { if (ly % 2 == 0) continue; }
                if (lx == 0 && lz == 0) continue;
                tree.push_back({ x + lx, y + ly, z + lz, BlockType::SpruceLeaves });
            }
        }
    }
    tree.push_back({ x, y + height, z, BlockType::SpruceLeaves });
    return tree;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetCactusAt(int x, int y, int z) {
    std::vector<TreeBlock> cactus;
    float n = Noise2D((float)x * 0.5f, (float)z * 0.5f);
    int height = 2 + (int)(std::abs(n) * 3);
    for (int i = 0; i < height; i++) cactus.push_back({ x, y + i, z, BlockType::Cactus });
    return cactus;
}

float WorldGenerator::GetContinentalness(int x, int z) {
    return FBM((float)x + seed, (float)z + seed, 2, 0.5f, 0.00005f) + 0.1f;
}

float WorldGenerator::GetPeaks(int x, int z) {
    float warpScale = 0.002f; float warpStr = 150.0f;
    float qx = Noise2D((float)x * warpScale + 5.2f, (float)z * warpScale + 1.3f);
    float qz = Noise2D((float)x * warpScale + 12.4f, (float)z * warpScale + 9.2f);
    float val = 0.0f;
    float n1 = 1.0f - std::abs(Noise2D((x + qx * warpStr) * 0.0015f, (z + qz * warpStr) * 0.0015f));
    val += n1;
    float n2 = 1.0f - std::abs(Noise2D((x + qx * warpStr) * 0.0045f, (z + qz * warpStr) * 0.0045f));
    val += n2 * 0.5f;
    return val * 0.6f;
}

float WorldGenerator::GetRiverFactor(int x, int z) {
    float n = FBM((float)x + (float)seed * 3.0f, (float)z + (float)seed * 3.0f, 4, 0.5f, 0.0008f);
    return 1.0f - std::abs(n);
}