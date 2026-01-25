#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "BlockType.h"

enum class BiomeType {
    Ocean,
    River,
    Beach,
    Scorched,
    Desert,
    Savanna,
    TropicalRainforest,
    TemperateDesert,
    TemperateDeciduousForest,
    TemperateRainforest,
    Taiga,
    Tundra,
    SnowyTundra,
    IceSpikes
};

struct BiomeSearchResult {
    bool found;
    int x;
    int z;
};

class WorldGenerator {
public:
    struct TreeBlock {
        int x, y, z;
        BlockType type;
    };

    WorldGenerator(int seed);

    void SetSeed(int seed);
    int GetSeed() const { return seed; }

    BlockType GetBlock(int x, int y, int z);
    int GetHeight(int x, int z);

    float GetTemperature(int x, int z);
    float GetHumidity(int x, int z);

    BiomeType GetBiome(int x, int z);
    std::string GetBiomeName(int x, int z);

    BiomeSearchResult FindBiome(int startX, int startZ, BiomeType target, int range, int step);
    BiomeType GetBiomeFromString(const std::string& name);

    float Noise2D(float x, float z);
    float Noise3D(float x, float y, float z);

    std::vector<TreeBlock> GetTreeAt(int x, int y, int z);
    std::vector<TreeBlock> GetSpruceAt(int x, int y, int z);
    std::vector<TreeBlock> GetCactusAt(int x, int y, int z);

private:
    int seed;
    int p[512];

    float Perlin(float x, float y, float z);
    void InitNoise();
    float Fade(float t);
    float Lerp(float t, float a, float b);
    float Grad(int hash, float x, float y, float z);
};