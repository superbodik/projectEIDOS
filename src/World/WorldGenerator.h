#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "BlockType.h"
#include "FastNoiseLite.h" // Убедись, что файл FastNoiseLite.h лежит в папке src/World

enum class BiomeType {
    Ocean, River, Beach, Scorched, Desert, Savanna, TropicalRainforest,
    TemperateDesert, TemperateDeciduousForest, TemperateRainforest,
    Taiga, Tundra, SnowyTundra, IceSpikes
};

struct BiomeSearchResult { bool found; int x; int z; };

class WorldGenerator {
public:
    struct TreeBlock { int x, y, z; BlockType type; };

    WorldGenerator(int seed);
    void SetSeed(int seed);
    int GetSeed() const { return seed; }

    BlockType GetBlock(int x, int y, int z);
    int GetHeight(int x, int z);
    float GetTemperature(int x, int z);
    float GetHumidity(int x, int z);

    BiomeType GetBiome(int x, int z);
    std::string GetBiomeName(int x, int z);
    BiomeType GetBiomeFromString(const std::string& name);
    BiomeSearchResult FindBiome(int startX, int startZ, BiomeType target, int range, int step);

private:
    int seed;
    FastNoiseLite noiseHeight;
    FastNoiseLite noiseBiome;
    FastNoiseLite noiseRiver;
    void InitNoise();
};