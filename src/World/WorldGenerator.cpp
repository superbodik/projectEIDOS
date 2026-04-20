#include "WorldGenerator.h"
#include <algorithm>
#include <cmath>

WorldGenerator::WorldGenerator(int seed) : seed(seed) { InitNoise(); }

void WorldGenerator::SetSeed(int s) { seed = s; InitNoise(); }

void WorldGenerator::InitNoise() {
    noiseHeight.SetSeed(seed);
    noiseHeight.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseHeight.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseHeight.SetFractalOctaves(4);
    noiseHeight.SetFrequency(0.005f);

    noiseBiome.SetSeed(seed + 123);
    noiseBiome.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseBiome.SetFrequency(0.002f);

    noiseRiver.SetSeed(seed + 456);
    noiseRiver.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseRiver.SetFrequency(0.005f);
}

int WorldGenerator::GetHeight(int x, int z) {
    float height = noiseHeight.GetNoise((float)x, (float)z);
    return (int)(65.0f + height * 30.0f);
}

BiomeType WorldGenerator::GetBiome(int x, int z) {
    float t = noiseBiome.GetNoise((float)x, (float)z);
    if (t < -0.5f) return BiomeType::Desert;
    if (t > 0.5f) return BiomeType::SnowyTundra;
    return BiomeType::TemperateDeciduousForest;
}

std::string WorldGenerator::GetBiomeName(int x, int z) {
    BiomeType b = GetBiome(x, z);
    if (b == BiomeType::Desert) return "Desert";
    if (b == BiomeType::SnowyTundra) return "Snow";
    return "Forest";
}

float WorldGenerator::GetTemperature(int x, int z) { (void)x; (void)z; return 0.5f; }
float WorldGenerator::GetHumidity(int x, int z) { (void)x; (void)z; return 0.5f; }
BiomeType WorldGenerator::GetBiomeFromString(const std::string& name) { (void)name; return BiomeType::Ocean; }
BiomeSearchResult WorldGenerator::FindBiome(int sX, int sZ, BiomeType t, int r, int st) {
    (void)sX; (void)sZ; (void)t; (void)r; (void)st;
    return { false,0,0 };
}


BlockType WorldGenerator::GetBlock(int x, int y, int z) {
    if (y < 0 || y >= 256) return BlockType::Air;
    if (y == 0) return BlockType::Bedrock;

    int h = GetHeight(x, z);

    if (y > h && y <= 63) return BlockType::Water;
    if (y > h) return BlockType::Air;

    if (y == h) {
        if (y <= 65) return BlockType::Sand;
        return BlockType::Grass;
    }
    if (y > h - 4) return BlockType::Dirt;

    return BlockType::Stone;
}