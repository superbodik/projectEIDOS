#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "BlockType.h"
#include "FastNoiseLite.h"

enum class BiomeType {
    Ocean, River, Beach, Scorched, Desert, Savanna, TropicalRainforest,
    TemperateDesert, TemperateDeciduousForest, TemperateRainforest,
    Taiga, Tundra, SnowyTundra, IceSpikes, Mountains,

    Swamp, Mangrove, BambooForest, SaltFlat,
    Steppe, Shrubland, BirchForest, RedwoodForest, Moorland,
    ColdDesert, AlpineMeadow, Glacier, VolcanicWastes,

    Volcano, Crag, HotSprings,

    Bayou, Bog, ConiferousForest, Floodplain, Highland
};

struct BiomeSearchResult { bool found; int x; int z; };

enum class RockSuite : unsigned char {
    Shield, Volcanic, Basin, Karst, Belt, Count
};

struct RockColumn {
    RockSuite suite = RockSuite::Shield;
    float     bedOffset = 0.0f;
    bool      veinActive = false;
    BlockType veinRock = BlockType::Quartzite;
    int       veinTop = 0;
    int       veinBottom = 0;
};

struct RiverInfo {
    bool  active = false;
    bool  channel = false;
    bool  plunge = false;
    bool  sill = false;
    bool  nearby = false;
    float strength = 0.0f;
    float dist = 0.0f;
    float halfWidth = 0.0f;
    float terrainH = 0.0f;
    int   waterLevel = 0;
    int   fallTop = 0;
};

struct ColumnInfo {
    int        height = 0;
    BiomeType  biome = BiomeType::Ocean;
    RiverInfo  river;
    RockColumn rock;
};

struct ClimateInfo {
    float airC = 15.0f;
    float baseC = 15.0f;
    float altitudeC = 0.0f;
    float diurnalC = 0.0f;
    float humidity = 0.5f;
    float latitude01 = 0.5f;
    bool  underground = false;
};

class WorldGenerator {
public:
    WorldGenerator(int seed);
    void SetSeed(int seed);
    int GetSeed() const { return seed; }

    BlockType GetBlock(int x, int y, int z);
    int GetHeight(int x, int z);

    ColumnInfo GetColumnInfo(int wx, int wz);
    BlockType  GetBlockFast(int x, int y, int z, const ColumnInfo& col);

    float    GetTemperature(int x, int z);
    float    GetHumidity(int x, int z);
    ClimateInfo GetClimate(int x, int y, int z, float timeOfDay, int surfaceY);

    static constexpr float COMFORT_MIN = 12.0f;
    static constexpr float COMFORT_MAX = 26.0f;

    BiomeType   GetBiome(int x, int z);
    std::string GetBiomeName(int x, int z);
    bool        IsRiver(int x, int z);
    bool        MatchesBiome(int x, int z, BiomeType target);
    BiomeSearchResult FindBiome(int startX, int startZ, BiomeType target, int range, int step);

    static bool ParseBiome(const std::string& name, BiomeType& out);
    static const std::vector<std::string>& BiomeNames();

    static constexpr int ROCK_PROBE_DEPTH = 8;

    struct GenSettings {
        float oreDensity = 1.0f;
        float caveDensity = 1.0f;
        float latitudeScale = 7000.0f;
    };

    void SetGenSettings(const GenSettings& s) { gen = s; }
    const GenSettings& GetGenSettings() const { return gen; }

    RockColumn  GetRockColumn(int x, int z);
    BlockType   GetSurfaceRock(int x, int z);
    static const char* RockSuiteName(RockSuite s);
    static bool IsCarbonate(BlockType r);
    static bool IsSedimentary(BlockType r);
    static bool IsIgneous(BlockType r);
    static bool IsMetamorphic(BlockType r);

    static constexpr int   SEA_LEVEL = 60;
    static constexpr float RIVER_STEP = 2.0f;
    static constexpr float RIVER_SILL_LEN = RIVER_STEP * 0.9f;
    static constexpr float RIVER_JET_LEN = RIVER_STEP * 1.5f;

    static constexpr float RIVER_NOTCH = 0.5f;
    static constexpr float RIVER_JET_MARGIN = 2.0f;

private:
    int seed;
    GenSettings gen;

    FastNoiseLite noiseHeight;
    FastNoiseLite noiseTemp;
    FastNoiseLite noiseHum;
    FastNoiseLite noiseMountains;
    FastNoiseLite noiseRidgedMount;
    FastNoiseLite noiseWarpX;
    FastNoiseLite noiseWarpZ;
    FastNoiseLite noiseErosion;
    FastNoiseLite noiseDetail;
    FastNoiseLite noiseCave;
    FastNoiseLite noiseCave2;
    FastNoiseLite noiseRiver;
    FastNoiseLite noiseRiverMeander;
    FastNoiseLite noiseRiverSize;
    FastNoiseLite noiseProvince;
    FastNoiseLite noiseBed;
    FastNoiseLite noiseVein;

    void InitNoise();

    float Hash(int x, int z)          const;
    float Hash3(int x, int y, int z)   const;

    float GetBaseHeight(int x, int z);

    float     RiverField(float x, float z) const;
    int       PoolLevel(float baseH) const;
    RiverInfo ComputeRiver(int x, int z, float baseH);

    bool IsCaveAt(int x, int y, int z, int surfH);
    bool CarvesAt(int x, int y, int z, int surfH, const RiverInfo& river);

    RockColumn ComputeRock(int x, int z);
    static BlockType BedRockAt(RockSuite suite, int y);

    BlockType GetStoneType(int x, int y, int z, const ColumnInfo& col) const;
    BlockType GetOreBlock(int x, int y, int z, BlockType host,
        const RockColumn& rc) const;
    BlockType PebbleForRock(BlockType rock) const;
    BlockType GetDecorationBlock(int x, int y, int z, const ColumnInfo& col);

    BlockType ResolveSurface(int x, int y, int z, const ColumnInfo& col);
    BlockType ResolveSubsurface(int x, int y, int z, int surfaceY, const ColumnInfo& col);
};
