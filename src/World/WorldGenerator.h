#pragma once
#include <vector>
#include "../World/BlockType.h"

class WorldGenerator {
public:
    int seed;

    const int SEA_LEVEL = 64;
    const int MOUNTAIN_LEVEL = 160;
    const int SNOW_LEVEL = 200;

    struct TreeBlock {
        int x, y, z;
        BlockType type;
    };

    WorldGenerator(int _seed = 0);
    void SetSeed(int newSeed) { seed = newSeed; }
    int GetSeed() const { return seed; }

    BlockType GetBlock(int x, int y, int z);
    int GetHeight(int x, int z);

    float Noise2D(float x, float z);
    float GetTemperature(int x, int z);
    float GetHumidity(int x, int z);

    std::vector<TreeBlock> GetTreeAt(int x, int y, int z);
    std::vector<TreeBlock> GetSpruceAt(int x, int y, int z);
    std::vector<TreeBlock> GetCactusAt(int x, int y, int z);

private:
    BlockType GetSurfaceBlock(int x, int z, int height, float temp, float hum);

    BlockType GetRockLayer(int x, int y, int z);
    BlockType GetOre(int x, int y, int z, BlockType rockType);

    float PseudoRandom(int x, int y);
    float RandomGradient(int ix, int iz);
    float FBM(float x, float z, int octaves, float persistence, float scale);

    float GetContinentalness(int x, int z);
    float GetPeaks(int x, int z);
    float GetRiverFactor(int x, int z);
};