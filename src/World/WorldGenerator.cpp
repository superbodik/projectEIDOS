#include "WorldGenerator.h"
#include <algorithm>
#include <numeric>
#include <random>

WorldGenerator::WorldGenerator(int seed) : seed(seed) {
    InitNoise();
}

void WorldGenerator::SetSeed(int s) {
    seed = s;
    InitNoise();
}

void WorldGenerator::InitNoise() {
    std::mt19937 gen(seed);
    std::iota(p, p + 256, 0);
    std::shuffle(p, p + 256, gen);
    for (int i = 0; i < 256; ++i) p[256 + i] = p[i];
}

float WorldGenerator::Noise2D(float x, float z) { return Perlin(x, 0.5f, z); }
float WorldGenerator::Noise3D(float x, float y, float z) { return Perlin(x, y, z); }

float WorldGenerator::Perlin(float x, float y, float z) {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;
    x -= floor(x); y -= floor(y); z -= floor(z);
    float u = Fade(x), v = Fade(y), w = Fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
    return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z), Grad(p[BA], x - 1, y, z)),
        Lerp(u, Grad(p[AB], x, y - 1, z), Grad(p[BB], x - 1, y - 1, z))),
        Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1), Grad(p[BA + 1], x - 1, y, z - 1)),
            Lerp(u, Grad(p[AB + 1], x, y - 1, z - 1), Grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

float WorldGenerator::Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float WorldGenerator::Lerp(float t, float a, float b) { return a + t * (b - a); }
float WorldGenerator::Grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float WorldGenerator::GetTemperature(int x, int z) {
    return (Noise2D(x * 0.002f, z * 0.002f) + 1.0f) * 0.5f;
}

float WorldGenerator::GetHumidity(int x, int z) {
    return (Noise2D(x * 0.003f + 1000, z * 0.003f + 1000) + 1.0f) * 0.5f;
}

BiomeType WorldGenerator::GetBiome(int x, int z) {
    int h = GetHeight(x, z);

    if (h < 60) return BiomeType::Ocean;

    float riverNoise = std::abs(Noise2D(x * 0.006f, z * 0.006f));
    if (h < 66 && riverNoise < 0.06f) return BiomeType::River;

    if (h >= 60 && h <= 64) return BiomeType::Beach;

    float temp = GetTemperature(x, z);
    float humidity = GetHumidity(x, z);

    if (temp < 0.25f) {
        if (humidity < 0.5f) return BiomeType::IceSpikes;
        return BiomeType::SnowyTundra;
    }
    else if (temp < 0.5f) {
        if (humidity < 0.3f) return BiomeType::Tundra;
        if (humidity < 0.7f) return BiomeType::Taiga;
        return BiomeType::TemperateRainforest;
    }
    else if (temp < 0.75f) {
        if (humidity < 0.3f) return BiomeType::TemperateDesert;
        if (humidity < 0.7f) return BiomeType::TemperateDeciduousForest;
        return BiomeType::TemperateRainforest;
    }
    else {
        if (humidity < 0.3f) return BiomeType::Scorched;
        if (humidity < 0.5f) return BiomeType::Desert;
        if (humidity < 0.8f) return BiomeType::Savanna;
        return BiomeType::TropicalRainforest;
    }
}

std::string WorldGenerator::GetBiomeName(int x, int z) {
    BiomeType b = GetBiome(x, z);
    switch (b) {
    case BiomeType::Ocean: return "Ocean";
    case BiomeType::River: return "River";
    case BiomeType::Beach: return "Beach";
    case BiomeType::Scorched: return "Scorched Wasteland";
    case BiomeType::Desert: return "Desert";
    case BiomeType::Savanna: return "Savanna";
    case BiomeType::TropicalRainforest: return "Tropical Rainforest";
    case BiomeType::TemperateDesert: return "Temperate Desert";
    case BiomeType::TemperateDeciduousForest: return "Forest";
    case BiomeType::TemperateRainforest: return "Wet Forest";
    case BiomeType::Taiga: return "Taiga";
    case BiomeType::Tundra: return "Tundra";
    case BiomeType::SnowyTundra: return "Snowy Tundra";
    case BiomeType::IceSpikes: return "Ice Plains";
    default: return "Unknown";
    }
}

BiomeType WorldGenerator::GetBiomeFromString(const std::string& name) {
    if (name == "Ocean") return BiomeType::Ocean;
    if (name == "River") return BiomeType::River;
    if (name == "Beach") return BiomeType::Beach;
    if (name == "Scorched") return BiomeType::Scorched;
    if (name == "Desert") return BiomeType::Desert;
    if (name == "Savanna") return BiomeType::Savanna;
    if (name == "TropicalRainforest" || name == "Jungle") return BiomeType::TropicalRainforest;
    if (name == "TemperateDesert") return BiomeType::TemperateDesert;
    if (name == "Forest") return BiomeType::TemperateDeciduousForest;
    if (name == "WetForest") return BiomeType::TemperateRainforest;
    if (name == "Taiga") return BiomeType::Taiga;
    if (name == "Tundra") return BiomeType::Tundra;
    if (name == "SnowyTundra") return BiomeType::SnowyTundra;
    if (name == "IceSpikes") return BiomeType::IceSpikes;
    return BiomeType::Ocean;
}

BiomeSearchResult WorldGenerator::FindBiome(int startX, int startZ, BiomeType target, int range, int step) {
    if (GetBiome(startX, startZ) == target) return { true, startX, startZ };

    int x = 0;
    int z = 0;
    int dx = 0;
    int dz = -1;

    int maxSteps = (range / step) * (range / step);

    for (int i = 0; i < maxSteps; i++) {
        int checkX = startX + (x * step);
        int checkZ = startZ + (z * step);

        if (GetBiome(checkX, checkZ) == target) {
            return { true, checkX, checkZ };
        }

        if (x == z || (x < 0 && x == -z) || (x > 0 && x == 1 - z)) {
            int t = dx;
            dx = -dz;
            dz = t;
        }
        x += dx;
        z += dz;
    }

    return { false, 0, 0 };
}

int WorldGenerator::GetHeight(int x, int z) {
    float base = Noise2D(x * 0.004f, z * 0.004f);
    float mountain = std::abs(Noise2D(x * 0.01f, z * 0.01f));
    float river = std::abs(Noise2D(x * 0.006f, z * 0.006f));

    float h = 65.0f;

    if (base < -0.3f) {
        h = 40.0f + (base * 20.0f);
    }
    else {
        float factor = (base + 0.3f);

        if (river < 0.1f) {
            float riverDepth = (0.1f - river) * 80.0f;
            h = (65.0f + (factor * 30.0f)) - riverDepth;
        }
        else {
            h = 65.0f + (factor * 40.0f) + (mountain * 60.0f * factor);
        }
    }

    return (int)h;
}

BlockType WorldGenerator::GetBlock(int x, int y, int z) {
    if (y < 0 || y >= 256) return BlockType::Air;
    if (y == 0) return BlockType::Bedrock;

    int surfaceH = GetHeight(x, z);

    if (y > surfaceH && y <= 63) return BlockType::Water;
    if (y > surfaceH) return BlockType::Air;

    BiomeType biome = GetBiome(x, z);

    if (y == surfaceH) {
        if (y <= 63 && biome != BiomeType::Ocean && biome != BiomeType::River) return BlockType::Sand;
        if (biome == BiomeType::Ocean) return BlockType::Gravel;
        if (biome == BiomeType::River) return BlockType::Clay;
        if (biome == BiomeType::Beach) return BlockType::Sand;

        if (biome == BiomeType::Desert || biome == BiomeType::Scorched) return BlockType::Sand;
        if (biome == BiomeType::TemperateDesert) return BlockType::RedSand;

        if (biome == BiomeType::SnowyTundra || biome == BiomeType::IceSpikes) return BlockType::Snow;

        return BlockType::Grass;
    }

    if (y > surfaceH - 4) {
        if (biome == BiomeType::Desert) return BlockType::Sand;
        if (biome == BiomeType::TemperateDesert) return BlockType::RedSand;
        if (biome == BiomeType::SnowyTundra || biome == BiomeType::IceSpikes) return BlockType::Snow;
        return BlockType::Dirt;
    }

    if (y < 15) return BlockType::Basalt;
    if (y < 40) return BlockType::Slate;
    return BlockType::Granite;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetTreeAt(int x, int y, int z) {
    std::vector<TreeBlock> tree;
    for (int i = 0; i < 5; i++) tree.push_back({ x, y + i, z, BlockType::OakLog });
    for (int lx = -2; lx <= 2; lx++) for (int lz = -2; lz <= 2; lz++) for (int ly = 3; ly <= 6; ly++) {
        if (std::abs(lx) + std::abs(lz) + std::abs(ly - 5) < 4) tree.push_back({ x + lx, y + ly, z + lz, BlockType::OakLeaves });
    }
    return tree;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetSpruceAt(int x, int y, int z) {
    std::vector<TreeBlock> tree;
    for (int i = 0; i < 7; i++) tree.push_back({ x, y + i, z, BlockType::SpruceLog });
    tree.push_back({ x, y + 7, z, BlockType::SpruceLeaves });
    for (int i = 2; i < 6; i += 2) {
        for (int lx = -1; lx <= 1; lx++) for (int lz = -1; lz <= 1; lz++) {
            if (std::abs(lx) + std::abs(lz) <= 2) tree.push_back({ x + lx, y + i, z + lz, BlockType::SpruceLeaves });
        }
    }
    return tree;
}

std::vector<WorldGenerator::TreeBlock> WorldGenerator::GetCactusAt(int x, int y, int z) {
    std::vector<TreeBlock> c;
    c.push_back({ x, y, z, BlockType::Cactus });
    c.push_back({ x, y + 1, z, BlockType::Cactus });
    c.push_back({ x, y + 2, z, BlockType::Cactus });
    return c;
}