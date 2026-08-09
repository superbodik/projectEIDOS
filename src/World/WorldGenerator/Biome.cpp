#include "../WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <climits>

BiomeType WorldGenerator::GetBiome(int x, int z) {
    float jT = (Hash(x * 3 + 11, z * 3 - 7) - 0.5f) * 0.024f;
    float jH = (Hash(x * 5 - 3, z * 5 + 17) - 0.5f) * 0.05f;

    float heightBase = (noiseHeight.GetNoise((float)x, (float)z) + 1.0f) * 0.5f;
    if (heightBase < 0.32f) return BiomeType::Ocean;
    if (heightBase < 0.38f + jT * 0.4f) return BiomeType::Beach;

    float temp = GetTemperature(x, z) + jT;
    float hum = GetHumidity(x, z) + jH;

    float mountainMask = (noiseMountains.GetNoise((float)x, (float)z) + 1.0f) * 0.5f + jT * 0.5f;
    if (mountainMask > 0.82f && heightBase > 0.52f) {
        float peak = (noiseDetail.GetNoise((float)x * 0.09f - 1900.0f,
            (float)z * 0.09f + 3300.0f) + 1.0f) * 0.5f
            + (Hash(x * 5 - 17, z * 5 + 23) - 0.5f) * 0.13f;
        if (temp > 0.70f && hum < 0.35f && peak < 0.14f) return BiomeType::Volcano;
        if (temp < 0.16f) return BiomeType::Glacier;
        if (temp > 0.50f && hum > 0.45f) return BiomeType::AlpineMeadow;
        if (peak > 0.62f) return BiomeType::Crag;
        return BiomeType::Mountains;
    }

    if (temp < 0.30f && hum > 0.32f && hum < 0.58f) {
        float vent = (noiseDetail.GetNoise((float)x * 0.21f + 7700.0f,
            (float)z * 0.21f - 5100.0f) + 1.0f) * 0.5f
            + (Hash(x * 9 + 61, z * 9 - 37) - 0.5f) * 0.10f;
        if (vent < 0.055f) return BiomeType::HotSprings;
    }

    float jR = (Hash(x * 7 + 29, z * 7 - 41) - 0.5f) * 0.07f;
    float jV = (Hash(x * 11 - 53, z * 11 + 67) - 0.5f) * 0.16f;
    float jL = (Hash(x * 13 + 97, z * 13 - 83) - 0.5f) * 0.020f;

    float relief = (noiseErosion.GetNoise((float)x * 0.55f, (float)z * 0.55f) + 1.0f) * 0.5f + jR;

    float variant = (noiseDetail.GetNoise((float)x * 0.16f + 4100.0f,
        (float)z * 0.16f - 2700.0f) + 1.0f) * 0.5f + jV;

    bool lowland = (heightBase < 0.455f + jL);

    if (temp > 0.75f) {
        if (hum < 0.12f && lowland && relief < 0.42f) return BiomeType::SaltFlat;
        if (hum < 0.20f) return (variant < 0.30f) ? BiomeType::VolcanicWastes
            : BiomeType::Scorched;
        if (hum < 0.40f) return BiomeType::Desert;
        if (hum < 0.62f) return (variant < 0.32f) ? BiomeType::Shrubland
            : BiomeType::Savanna;
        if (hum > 0.86f && lowland) return (variant < 0.45f) ? BiomeType::Floodplain
            : BiomeType::Mangrove;
        if (hum > 0.74f && lowland && relief < 0.44f) return BiomeType::Bayou;
        return (variant < 0.28f) ? BiomeType::BambooForest
            : BiomeType::TropicalRainforest;
    }
    if (temp > 0.45f) {
        if (hum < 0.22f) return BiomeType::TemperateDesert;
        if (hum < 0.38f) return BiomeType::Steppe;
        if (relief > 0.66f && variant > 0.55f) return BiomeType::Highland;
        if (hum > 0.82f && lowland && relief < 0.46f) return BiomeType::Swamp;
        if (hum < 0.70f) return (variant < 0.30f) ? BiomeType::BirchForest
            : BiomeType::TemperateDeciduousForest;
        return (variant < 0.26f) ? BiomeType::RedwoodForest
            : BiomeType::TemperateRainforest;
    }
    if (temp > 0.20f) {
        if (hum < 0.24f) return BiomeType::ColdDesert;
        if (hum < 0.50f) return BiomeType::Tundra;
        if (hum > 0.78f && lowland && relief < 0.42f) return BiomeType::Bog;
        if (hum > 0.78f && lowland) return BiomeType::Moorland;
        return (variant < 0.34f) ? BiomeType::ConiferousForest : BiomeType::Taiga;
    }
    if (hum < 0.40f) return BiomeType::SnowyTundra;
    return BiomeType::IceSpikes;
}


std::string WorldGenerator::GetBiomeName(int x, int z) {
    if (ComputeRiver(x, z, GetBaseHeight(x, z)).channel) return "River";

    switch (GetBiome(x, z)) {
    case BiomeType::Ocean:                    return "Ocean";
    case BiomeType::Beach:                    return "Beach";
    case BiomeType::Scorched:                 return "Scorched Desert";
    case BiomeType::Desert:                   return "Desert";
    case BiomeType::Savanna:                  return "Savanna";
    case BiomeType::TropicalRainforest:       return "Jungle";
    case BiomeType::TemperateDesert:          return "Dry Plains";
    case BiomeType::TemperateDeciduousForest: return "Forest";
    case BiomeType::TemperateRainforest:      return "Rainforest";
    case BiomeType::Taiga:                    return "Taiga";
    case BiomeType::Tundra:                   return "Tundra";
    case BiomeType::SnowyTundra:              return "Snowy Tundra";
    case BiomeType::IceSpikes:                return "Ice Spikes";
    case BiomeType::Mountains:                return "Mountains";
    case BiomeType::Swamp:                    return "Swamp";
    case BiomeType::Mangrove:                 return "Mangrove";
    case BiomeType::BambooForest:             return "Bamboo Forest";
    case BiomeType::SaltFlat:                 return "Salt Flat";
    case BiomeType::Steppe:                   return "Steppe";
    case BiomeType::Shrubland:                return "Shrubland";
    case BiomeType::BirchForest:              return "Birch Forest";
    case BiomeType::RedwoodForest:            return "Redwood Forest";
    case BiomeType::Moorland:                 return "Moorland";
    case BiomeType::ColdDesert:               return "Cold Desert";
    case BiomeType::AlpineMeadow:             return "Alpine Meadow";
    case BiomeType::Glacier:                  return "Glacier";
    case BiomeType::VolcanicWastes:           return "Volcanic Wastes";
    case BiomeType::Volcano:                  return "Volcano";
    case BiomeType::Crag:                     return "Crag";
    case BiomeType::HotSprings:               return "Hot Springs";
    case BiomeType::Bayou:                    return "Bayou";
    case BiomeType::Bog:                      return "Bog";
    case BiomeType::ConiferousForest:         return "Coniferous Forest";
    case BiomeType::Floodplain:               return "Floodplain";
    case BiomeType::Highland:                 return "Highland";
    default:                                   return "Unknown";
    }
}

namespace {
    struct BiomeAlias { const char* key; BiomeType type; };

    const BiomeAlias kBiomeAliases[] = {
    { "bayou", BiomeType::Bayou }, { "willowswamp", BiomeType::Bayou },
    { "bog", BiomeType::Bog }, { "cranberrybog", BiomeType::Bog },
    { "coniferousforest", BiomeType::ConiferousForest },
    { "conifer", BiomeType::ConiferousForest }, { "firforest", BiomeType::ConiferousForest },
    { "floodplain", BiomeType::Floodplain }, { "flood", BiomeType::Floodplain },
    { "highland", BiomeType::Highland }, { "highlands", BiomeType::Highland },
    { "volcano", BiomeType::Volcano }, { "caldera", BiomeType::Volcano },
    { "crag", BiomeType::Crag }, { "crags", BiomeType::Crag },
    { "hotsprings", BiomeType::HotSprings }, { "geyser", BiomeType::HotSprings },
    { "springs", BiomeType::HotSprings },
    { "swamp", BiomeType::Swamp }, { "marsh", BiomeType::Swamp },
    { "bog", BiomeType::Swamp },
    { "mangrove", BiomeType::Mangrove }, { "mangroves", BiomeType::Mangrove },
    { "bambooforest", BiomeType::BambooForest }, { "bamboo", BiomeType::BambooForest },
    { "saltflat", BiomeType::SaltFlat }, { "salt", BiomeType::SaltFlat },
    { "steppe", BiomeType::Steppe }, { "grassland", BiomeType::Steppe },
    { "prairie", BiomeType::Steppe }, { "plains", BiomeType::Steppe },
    { "shrubland", BiomeType::Shrubland }, { "chaparral", BiomeType::Shrubland },
    { "scrub", BiomeType::Shrubland },
    { "birchforest", BiomeType::BirchForest }, { "birch", BiomeType::BirchForest },
    { "redwoodforest", BiomeType::RedwoodForest }, { "redwood", BiomeType::RedwoodForest },
    { "sequoia", BiomeType::RedwoodForest },
    { "moorland", BiomeType::Moorland }, { "moor", BiomeType::Moorland },
    { "heath", BiomeType::Moorland },
    { "colddesert", BiomeType::ColdDesert },
    { "alpinemeadow", BiomeType::AlpineMeadow }, { "alpine", BiomeType::AlpineMeadow },
    { "meadow", BiomeType::AlpineMeadow },
    { "glacier", BiomeType::Glacier }, { "icecap", BiomeType::Glacier },
    { "volcanicwastes", BiomeType::VolcanicWastes },
    { "volcanic", BiomeType::VolcanicWastes }, { "lavafields", BiomeType::VolcanicWastes },
        { "ocean", BiomeType::Ocean },
        { "sea", BiomeType::Ocean },
        { "river", BiomeType::River },
        { "beach", BiomeType::Beach },
        { "shore", BiomeType::Beach },
        { "coast", BiomeType::Beach },
        { "scorched", BiomeType::Scorched },
        { "scorcheddesert", BiomeType::Scorched },
        { "badlands", BiomeType::Scorched },
        { "desert", BiomeType::Desert },
        { "savanna", BiomeType::Savanna },
        { "savannah", BiomeType::Savanna },
        { "jungle", BiomeType::TropicalRainforest },
        { "tropicalrainforest", BiomeType::TropicalRainforest },
        { "tropical", BiomeType::TropicalRainforest },
        { "temperatedesert", BiomeType::TemperateDesert },
        { "dryplains", BiomeType::TemperateDesert },
        { "plains", BiomeType::TemperateDesert },
        { "steppe", BiomeType::TemperateDesert },
        { "forest", BiomeType::TemperateDeciduousForest },
        { "temperatedeciduousforest", BiomeType::TemperateDeciduousForest },
        { "deciduousforest", BiomeType::TemperateDeciduousForest },
        { "deciduous", BiomeType::TemperateDeciduousForest },
        { "oakforest", BiomeType::TemperateDeciduousForest },
        { "rainforest", BiomeType::TemperateRainforest },
        { "temperaterainforest", BiomeType::TemperateRainforest },
        { "taiga", BiomeType::Taiga },
        { "borealforest", BiomeType::Taiga },
        { "boreal", BiomeType::Taiga },
        { "tundra", BiomeType::Tundra },
        { "snowytundra", BiomeType::SnowyTundra },
        { "snowy", BiomeType::SnowyTundra },
        { "snow", BiomeType::SnowyTundra },
        { "icespikes", BiomeType::IceSpikes },
        { "spikes", BiomeType::IceSpikes },
        { "ice", BiomeType::IceSpikes },
        { "mountains", BiomeType::Mountains },
        { "mountain", BiomeType::Mountains },
        { "peaks", BiomeType::Mountains },
    };

    std::string NormalizeBiomeKey(const std::string& s) {
        std::string r;
        r.reserve(s.size());
        for (unsigned char c : s) {
            if (c == ' ' || c == '_' || c == '-') continue;
            r += (char)std::tolower(c);
        }
        return r;
    }
}

const std::vector<std::string>& WorldGenerator::BiomeNames() {
    static const std::vector<std::string> names = {
        "Ocean", "River", "Beach", "Scorched", "Desert", "Savanna", "Jungle",
        "TemperateDesert", "Forest", "Rainforest", "Taiga", "Tundra",
        "SnowyTundra", "IceSpikes", "Mountains",
        "Swamp", "Mangrove", "BambooForest", "SaltFlat", "Steppe", "Shrubland",
        "BirchForest", "RedwoodForest", "Moorland", "ColdDesert",
        "AlpineMeadow", "Glacier", "VolcanicWastes",
        "Volcano", "Crag", "HotSprings",
        "Bayou", "Bog", "ConiferousForest", "Floodplain", "Highland"
    };
    return names;
}

bool WorldGenerator::ParseBiome(const std::string& name, BiomeType& out) {
    std::string key = NormalizeBiomeKey(name);
    if (key.empty()) return false;
    for (const BiomeAlias& a : kBiomeAliases) {
        if (key == a.key) { out = a.type; return true; }
    }
    return false;
}


bool WorldGenerator::IsRiver(int x, int z) {
    return ComputeRiver(x, z, GetBaseHeight(x, z)).channel;
}

bool WorldGenerator::MatchesBiome(int x, int z, BiomeType target) {
    if (target == BiomeType::River) return IsRiver(x, z);
    if (GetBiome(x, z) != target) return false;
    return !IsRiver(x, z);
}

BiomeSearchResult WorldGenerator::FindBiome(int startX, int startZ,
    BiomeType target, int range, int step) {
    if (step < 1) step = 1;
    if (MatchesBiome(startX, startZ, target)) return { true, startX, startZ };

    for (int r = step; r < range; r += step) {
        for (int d = -r; d <= r; d += step) {
            if (MatchesBiome(startX + d, startZ - r, target)) return { true, startX + d, startZ - r };
            if (MatchesBiome(startX + d, startZ + r, target)) return { true, startX + d, startZ + r };
            if (MatchesBiome(startX - r, startZ + d, target)) return { true, startX - r, startZ + d };
            if (MatchesBiome(startX + r, startZ + d, target)) return { true, startX + r, startZ + d };
        }
    }
    return { false, 0, 0 };
}

