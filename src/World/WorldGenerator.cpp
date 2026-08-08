#include "WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <climits>

WorldGenerator::WorldGenerator(int seed) : seed(seed) { InitNoise(); }
void WorldGenerator::SetSeed(int s) { seed = s; InitNoise(); }

void WorldGenerator::InitNoise() {
    noiseHeight.SetSeed(seed);
    noiseHeight.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseHeight.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseHeight.SetFractalOctaves(6);
    noiseHeight.SetFractalLacunarity(2.1f);
    noiseHeight.SetFractalGain(0.45f);
    noiseHeight.SetFrequency(0.0007f);

    noiseMountains.SetSeed(seed + 789);
    noiseMountains.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseMountains.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseMountains.SetFractalOctaves(3);
    noiseMountains.SetFrequency(0.0009f);

    noiseRidgedMount.SetSeed(seed + 101);
    noiseRidgedMount.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noiseRidgedMount.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noiseRidgedMount.SetFractalOctaves(6);
    noiseRidgedMount.SetFractalLacunarity(2.0f);
    noiseRidgedMount.SetFractalGain(0.5f);
    noiseRidgedMount.SetFrequency(0.0018f);

    noiseTemp.SetSeed(seed + 123);
    noiseTemp.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseTemp.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseTemp.SetFractalOctaves(2);
    noiseTemp.SetFrequency(0.0004f);

    noiseHum.SetSeed(seed + 456);
    noiseHum.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseHum.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseHum.SetFractalOctaves(3);
    noiseHum.SetFrequency(0.0006f);

    noiseWarpX.SetSeed(seed + 1001);
    noiseWarpX.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseWarpX.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseWarpX.SetFractalOctaves(3);
    noiseWarpX.SetFrequency(0.0008f);

    noiseWarpZ.SetSeed(seed + 1002);
    noiseWarpZ.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseWarpZ.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseWarpZ.SetFractalOctaves(3);
    noiseWarpZ.SetFrequency(0.0008f);

    noiseErosion.SetSeed(seed + 3001);
    noiseErosion.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseErosion.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseErosion.SetFractalOctaves(4);
    noiseErosion.SetFrequency(0.0025f);

    noiseDetail.SetSeed(seed + 4001);
    noiseDetail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseDetail.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseDetail.SetFractalOctaves(3);
    noiseDetail.SetFrequency(0.006f);

    noiseCave.SetSeed(seed + 2001);
    noiseCave.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noiseCave.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noiseCave.SetFractalOctaves(2);
    noiseCave.SetFrequency(0.009f);

    noiseCave2.SetSeed(seed + 2002);
    noiseCave2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noiseCave2.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noiseCave2.SetFractalOctaves(2);
    noiseCave2.SetFrequency(0.007f);

    noiseRiver.SetSeed(seed + 5001);
    noiseRiver.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseRiver.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseRiver.SetFractalOctaves(2);
    noiseRiver.SetFractalLacunarity(2.0f);
    noiseRiver.SetFractalGain(0.45f);
    noiseRiver.SetFrequency(0.00045f);

    noiseRiverMeander.SetSeed(seed + 5002);
    noiseRiverMeander.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseRiverMeander.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseRiverMeander.SetFractalOctaves(2);
    noiseRiverMeander.SetFrequency(0.0025f);

    noiseRiverSize.SetSeed(seed + 5003);
    noiseRiverSize.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseRiverSize.SetFractalOctaves(2);
    noiseRiverSize.SetFrequency(0.00012f);

    noiseProvince.SetSeed(seed + 7001);
    noiseProvince.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    noiseProvince.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
    noiseProvince.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    noiseProvince.SetFrequency(0.0013f);

    noiseBed.SetSeed(seed + 7002);
    noiseBed.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseBed.SetFractalType(FastNoiseLite::FractalType_FBm);
    noiseBed.SetFractalOctaves(3);
    noiseBed.SetFrequency(0.0016f);

    noiseVein.SetSeed(seed + 7003);
    noiseVein.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseVein.SetFractalType(FastNoiseLite::FractalType_Ridged);
    noiseVein.SetFractalOctaves(3);
    noiseVein.SetFrequency(0.0042f);
}

float WorldGenerator::Hash(int x, int z) const {
    int h = seed + x * 374761393 + z * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return (float)(h & 0x0FFFFFFF) / (float)0x0FFFFFFF;
}

float WorldGenerator::Hash3(int x, int y, int z) const {
    int h = seed + x * 374761393 + y * 1274126177 + z * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    h ^= (h >> 16);
    return (float)(h & 0x0FFFFFFF) / (float)0x0FFFFFFF;
}

float WorldGenerator::GetTemperature(int x, int z) {
    float scale = std::max(500.0f, gen.latitudeScale);
    float latitude = 1.0f - std::clamp(std::abs((float)z) / scale, 0.0f, 1.0f);
    float noise = (noiseTemp.GetNoise((float)x, (float)z) + 1.0f) * 0.5f;
    return std::clamp(latitude * 0.85f + noise * 0.15f, 0.0f, 1.0f);
}

float WorldGenerator::GetHumidity(int x, int z) {
    return (noiseHum.GetNoise((float)x, (float)z) + 1.0f) * 0.5f;
}

ClimateInfo WorldGenerator::GetClimate(int x, int y, int z, float timeOfDay, int surfaceY) {
    ClimateInfo c;
    c.latitude01 = GetTemperature(x, z);
    c.humidity = GetHumidity(x, z);

    c.baseC = std::lerp(-30.0f, 38.0f, c.latitude01);

    int ground = std::max(surfaceY, SEA_LEVEL);
    float above = (float)std::max(0, ground - SEA_LEVEL);
    c.altitudeC = -above * 0.34f;

    float swingAmp = std::lerp(15.0f, 4.5f, std::clamp(c.humidity, 0.0f, 1.0f));
    float dayPhase = (timeOfDay - 0.30f) * 6.2831853f;
    c.diurnalC = sinf(dayPhase) * swingAmp;

    float surfaceC = c.baseC + c.altitudeC + c.diurnalC;

    int depth = surfaceY - y;
    if (depth > 5) {
        c.underground = true;
        float k = std::clamp((float)(depth - 5) / 22.0f, 0.0f, 1.0f);
        float caveC = 11.0f + std::max(0.0f, 34.0f - (float)y) * 0.75f;
        c.airC = std::lerp(surfaceC, caveC, k);
    }
    else {
        c.airC = surfaceC;
    }

    return c;
}

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

float WorldGenerator::GetBaseHeight(int x, int z) {
    const float WF = 0.0008f, WS = 160.0f;
    float wx = (float)x + noiseWarpX.GetNoise((float)x * WF, (float)z * WF) * WS;
    float wz = (float)z + noiseWarpZ.GetNoise((float)x * WF, (float)z * WF) * WS;
    const float WF2 = WF * 2.0f, WS2 = 55.0f;
    wx += noiseWarpZ.GetNoise(wx * WF2, wz * WF2) * WS2;
    wz += noiseWarpX.GetNoise(wx * WF2, wz * WF2) * WS2;

    float temp = GetTemperature(x, z);
    float hum = GetHumidity(x, z);
    float baseNoise = noiseHeight.GetNoise(wx, wz);

    float baseH = 64.0f + temp * 6.0f - hum * 3.0f;
    float amp = 14.0f + hum * 18.0f + temp * 5.0f;

    if (baseNoise < -0.05f) {
        float b = std::clamp((-0.05f - baseNoise) * 4.5f, 0.0f, 1.0f);
        baseH = std::lerp(baseH, 28.0f, b);
        amp = std::lerp(amp, 7.0f, b);
    }

    float mntMask = (noiseMountains.GetNoise(wx, wz) + 1.0f) * 0.5f;
    if (mntMask > 0.55f) {
        float b = std::clamp((mntMask - 0.55f) * 2.22f, 0.0f, 1.0f);
        b = b * b * (3.0f - 2.0f * b);
        float ridged = noiseRidgedMount.GetNoise(wx, wz);
        baseH = std::lerp(baseH, 95.0f + ridged * 70.0f, b);
        amp = std::lerp(amp, 90.0f, b);
    }

    float erosion = (noiseErosion.GetNoise(wx, wz) + 1.0f) * 0.5f;
    amp *= (1.0f - std::clamp((erosion - 0.50f) * 2.5f, 0.0f, 0.78f));

    float detail = noiseDetail.GetNoise(wx, wz) * 2.8f;
    return std::max(0.0f, baseH + baseNoise * amp + detail);
}

float WorldGenerator::RiverField(float x, float z) const {
    float mx = noiseRiverMeander.GetNoise(x, z) * 38.0f;
    float mz = noiseRiverMeander.GetNoise(x + 4213.0f, z - 1877.0f) * 38.0f;
    return noiseRiver.GetNoise(x + mx, z + mz);
}

int WorldGenerator::PoolLevel(float baseH) const {
    int step = (int)RIVER_STEP;
    int lvl = (int)std::floor((baseH - 1.0f) / RIVER_STEP) * step;
    return std::max(lvl, SEA_LEVEL);
}

RiverInfo WorldGenerator::ComputeRiver(int x, int z, float baseH) {
    RiverInfo r;
    r.terrainH = baseH;

    if (baseH < 45.0f || baseH > 170.0f) return r;

    float fx = (float)x, fz = (float)z;
    float f = RiverField(fx, fz);

    const float E = 4.0f;
    float gx = (RiverField(fx + E, fz) - RiverField(fx - E, fz)) / (2.0f * E);
    float gz = (RiverField(fx, fz + E) - RiverField(fx, fz - E)) / (2.0f * E);
    float g = std::sqrt(gx * gx + gz * gz);
    if (g < 1e-7f) return r;
    float dist = std::abs(f) / g;

    if (dist > 28.0f) return r;

    const int   D = 4;
    const float DF = (float)D;
    float hE = GetBaseHeight(x + D, z), hW = GetBaseHeight(x - D, z);
    float hN = GetBaseHeight(x, z + D), hS = GetBaseHeight(x, z - D);
    float sx = (hE - hW) / (2.0f * DF);
    float sz = (hN - hS) / (2.0f * DF);
    float slope = std::sqrt(sx * sx + sz * sz);
    float refH = (baseH * 2.0f + hE + hW + hN + hS) / 6.0f;

    float steepFade = 1.0f - std::clamp((slope - 0.34f) / 0.16f, 0.0f, 1.0f);

    float sizeN = (noiseRiverSize.GetNoise(fx, fz) + 1.0f) * 0.5f;
    float alt = std::clamp((refH - 63.0f) / 55.0f, 0.0f, 1.0f);
    float flow = std::lerp(1.40f, 0.42f, alt);

    float halfW = std::clamp((2.0f + sizeN * 6.0f) * flow, 1.2f, 12.0f) * steepFade;
    float bankW = (halfW + 4.5f + sizeN * 6.0f) * steepFade;

    bool canHost = (baseH >= (float)SEA_LEVEL - 1.5f && baseH <= 155.0f);
    if (!canHost || dist > bankW || bankW <= halfW + 0.01f) {

        if (dist < bankW + 8.0f) {
            r.nearby = true;
            r.waterLevel = PoolLevel(refH);
        }
        return r;
    }
    r.nearby = true;

    int waterLevel = PoolLevel(refH);

    float depth = (2.0f + sizeN * 3.5f) * std::lerp(1.0f, 0.65f, alt);

    r.active = true;
    r.dist = dist;
    r.waterLevel = waterLevel;
    r.fallTop = waterLevel;
    r.halfWidth = std::max(halfW, 2.5f);
    r.channel = (halfW > 0.01f && dist <= halfW);
    if (r.channel) {
        float t = dist / halfW;
        r.strength = std::sqrt(std::max(0.0f, 1.0f - t * t));
    }

    float band = RIVER_STEP / std::max(slope, 0.02f);
    float q = (refH - 1.0f) / RIVER_STEP;
    float frac = q - std::floor(q);

    r.sill = (frac * band < RIVER_SILL_LEN);
    if ((1.0f - frac) * band < RIVER_JET_LEN) {
        r.fallTop = waterLevel + (int)RIVER_STEP;
        r.plunge = true;
        depth += RIVER_STEP * 0.75f;
    }

    if (r.channel) {
        r.terrainH = (float)waterLevel - (1.0f + (depth - 1.0f) * r.strength);
    }
    else {

        float t = (dist - halfW) / (bankW - halfW);
        t = t * t * (3.0f - 2.0f * t);
        r.terrainH = std::lerp((float)waterLevel - 1.0f, baseH, t);
    }

    if (r.sill)
        r.terrainH = std::max(r.terrainH,
            (float)waterLevel - (dist <= std::min(halfW * RIVER_NOTCH, 2.0f) ? 1.0f : 0.0f));

    r.terrainH = std::max(1.0f, r.terrainH);
    return r;
}

int WorldGenerator::GetHeight(int x, int z) {
    float baseH = GetBaseHeight(x, z);
    RiverInfo r = ComputeRiver(x, z, baseH);
    return std::max(0, (int)r.terrainH);
}

ColumnInfo WorldGenerator::GetColumnInfo(int wx, int wz) {
    ColumnInfo c;
    float baseH = GetBaseHeight(wx, wz);
    c.river = ComputeRiver(wx, wz, baseH);
    c.height = std::max(0, (int)c.river.terrainH);
    c.biome = GetBiome(wx, wz);
    c.rock = ComputeRock(wx, wz);
    return c;
}

BlockType WorldGenerator::ResolveSurface(int x, int y, int z, const ColumnInfo& col) {
    const RiverInfo& r = col.river;
    const BiomeType  biome = col.biome;

    if (r.active) {
        float h = Hash3(x, y, z);
        bool arid = (biome == BiomeType::Desert || biome == BiomeType::Scorched ||
            biome == BiomeType::TemperateDesert);

        if (r.channel) {
            if (r.sill) return GetStoneType(x, y, z, col);
            if (r.plunge || r.strength < 0.35f) return (h < 0.62f) ? BlockType::Gravel : BlockType::Sand;
            if (arid)      return (h < 0.20f) ? BlockType::Gravel : BlockType::Sand;
            if (h < 0.28f) return BlockType::Gravel;
            if (h < 0.70f) return BlockType::Sand;
            return BlockType::Clay;
        }

        if (y <= r.waterLevel + 2) {
            if (biome == BiomeType::Scorched) return BlockType::RedSand;
            return (h < 0.25f) ? BlockType::Gravel : BlockType::Sand;
        }
    }

    if (y <= 61 && biome != BiomeType::Desert && biome != BiomeType::Scorched)
        return BlockType::Sand;
    switch (biome) {
    case BiomeType::Mountains:
        return (y > 130) ? BlockType::Snow : GetStoneType(x, y, z, col);
    case BiomeType::Scorched:             return BlockType::RedSand;
    case BiomeType::Desert:
    case BiomeType::TemperateDesert:
    case BiomeType::Beach:                return BlockType::Sand;
    case BiomeType::Tundra:
    case BiomeType::SnowyTundra:
    case BiomeType::IceSpikes:            return BlockType::Snow;
    case BiomeType::TropicalRainforest:   return BlockType::Mud;
    case BiomeType::Taiga:                return BlockType::CoarseDirt;
    case BiomeType::Swamp:
    case BiomeType::Mangrove:             return BlockType::Mud;
    case BiomeType::Moorland:             return BlockType::Peat;
    case BiomeType::Bog:                  return BlockType::Peat;
    case BiomeType::Bayou:
    case BiomeType::Floodplain:           return BlockType::Mud;
    case BiomeType::SaltFlat:             return BlockType::Chalk;
    case BiomeType::VolcanicWastes:       return BlockType::Basalt;
    case BiomeType::Volcano: {
        float v = Hash3(x, y, z);
        if (v > 0.972f) return BlockType::Lava;
        return (v > 0.55f) ? BlockType::Basalt : BlockType::Gabbro;
    }
    case BiomeType::Crag: {
        float v = Hash3(x + 41, y, z - 17);
        if (v > 0.74f) return BlockType::Cobblestone;
        if (v > 0.58f) return BlockType::Andesite;
        return BlockType::Grass;
    }
    case BiomeType::HotSprings: {
        float v = Hash3(x - 23, y, z + 61);
        return (v > 0.62f) ? BlockType::Marble : BlockType::Chalk;
    }
    case BiomeType::Glacier:              return BlockType::PackedIce;
    case BiomeType::ColdDesert:           return BlockType::Gravel;
    case BiomeType::Steppe:
    case BiomeType::Shrubland:            return BlockType::CoarseDirt;
    default:                               return BlockType::Grass;
    }
}

BlockType WorldGenerator::ResolveSubsurface(int x, int y, int z, int surfaceY,
    const ColumnInfo& col) {
    const BiomeType biome = col.biome;
    int depth = surfaceY - y;
    if (depth == 1) {
        if (biome == BiomeType::Desert || biome == BiomeType::Beach ||
            biome == BiomeType::TemperateDesert) return BlockType::Sand;
        if (biome == BiomeType::Scorched)          return BlockType::RedSand;
        if (biome == BiomeType::TropicalRainforest) return BlockType::Mud;
        return BlockType::Dirt;
    }
    if (depth <= 4) {
        if (biome == BiomeType::Desert || biome == BiomeType::Beach) return BlockType::Sand;
        if (biome == BiomeType::Scorched)                             return BlockType::RedSand;
        return BlockType::Dirt;
    }
    if (depth <= 7) {
        float h = Hash3(x, y, z);
        if (biome == BiomeType::TropicalRainforest && h > 0.78f) return BlockType::Clay;
        if (biome == BiomeType::Beach && h > 0.72f)               return BlockType::Gravel;
        return GetStoneType(x, y, z, col);
    }
    return GetOreBlock(x, y, z, GetStoneType(x, y, z, col), col.rock);
}

BlockType WorldGenerator::GetBlockFast(int x, int y, int z, const ColumnInfo& col) {
    const int        surfH = col.height;
    const BiomeType  biome = col.biome;
    const RiverInfo& river = col.river;

    if (y < 0 || y >= 256) return BlockType::Air;
    if (y == 0)             return BlockType::Bedrock;
    if (y <= 4) {
        float bh = Hash3(x, y, z);
        if (y == 1 && bh > 0.30f) return BlockType::Bedrock;
        if (y == 2 && bh > 0.60f) return BlockType::Bedrock;
        if (y == 3 && bh > 0.82f) return BlockType::Bedrock;
        if (y == 4 && bh > 0.95f) return BlockType::Bedrock;
    }

    if (y > surfH && y <= SEA_LEVEL) {
        if (biome == BiomeType::SnowyTundra || biome == BiomeType::IceSpikes)
            return (y == SEA_LEVEL) ? BlockType::Ice : BlockType::Water;
        return BlockType::Water;
    }

    if (river.active && y > surfH) {
        bool frozen = (biome == BiomeType::SnowyTundra || biome == BiomeType::IceSpikes ||
            biome == BiomeType::Tundra);
        if (y <= river.waterLevel)
            return (y == river.waterLevel && frozen) ? BlockType::Ice : BlockType::Water;

        if (y <= river.fallTop &&
            river.dist <= river.halfWidth * RIVER_NOTCH + RIVER_JET_MARGIN)
            return BlockType::Water;
    }

    if (y > surfH) return GetDecorationBlock(x, y, z, col);

    if (y > 4 && y <= surfH && CarvesAt(x, y, z, surfH, river))
        return (y <= 11) ? BlockType::Lava : BlockType::Air;

    if (y == surfH && y > 5 && CarvesAt(x, y - 1, z, surfH, river))
        return BlockType::Air;

    if (y == surfH) return ResolveSurface(x, y, z, col);
    return ResolveSubsurface(x, y, z, surfH, col);
}

bool WorldGenerator::IsCaveAt(int x, int y, int z, int surfH) {
    if (y <= 4 || y > surfH) return false;

    float density = std::clamp(gen.caveDensity, 0.0f, 2.0f);
    if (density <= 0.001f) return false;
    float relax = (density - 1.0f) * 0.06f;

    float fx = (float)x, fy = (float)y, fz = (float)z;
    float cn1 = noiseCave.GetNoise(fx, fy * 2.1f, fz);
    float cn2 = noiseCave2.GetNoise(fx, fy * 2.1f, fz);

    float depth = (float)(surfH - y);
    float wide = std::clamp(depth / 40.0f, 0.0f, 1.0f);
    float thr = 0.86f - wide * 0.07f - relax;
    if (cn1 > thr && cn2 > thr) return true;

    if (y < 34) {
        float room = noiseCave.GetNoise(fx * 0.35f, fy * 0.55f, fz * 0.35f);
        if (room > 0.90f - relax + (float)std::max(0, y - 12) * 0.003f) return true;
    }

    if (depth > 6.0f) {
        float rav = noiseCave2.GetNoise(fx * 0.13f, fz * 0.13f);
        if (rav > 0.90f - relax && cn1 > 0.72f - relax) return true;
    }

    return false;
}

bool WorldGenerator::CarvesAt(int x, int y, int z, int surfH, const RiverInfo& river) {
    if (river.nearby && y > river.waterLevel - 24) return false;
    if (surfH <= SEA_LEVEL + 1 && y > surfH - 12) return false;
    return IsCaveAt(x, y, z, surfH);
}

BlockType WorldGenerator::GetBlock(int x, int y, int z) {
    return GetBlockFast(x, y, z, GetColumnInfo(x, z));
}

namespace {

struct Bed { int top; BlockType rock; };

const Bed BEDS_SHIELD[] = {
    { 92, BlockType::Quartzite }, { 68, BlockType::Gneiss }, { 44, BlockType::Schist },
    { 24, BlockType::Granite },   { 10, BlockType::Diorite }, { INT_MIN, BlockType::Gabbro }
};
const Bed BEDS_VOLCANIC[] = {
    { 98, BlockType::Rhyolite }, { 76, BlockType::Dacite }, { 52, BlockType::Andesite },
    { 28, BlockType::Basalt },   { 12, BlockType::Gabbro }, { INT_MIN, BlockType::Granite }
};
const Bed BEDS_BASIN[] = {
    { 110, BlockType::Claystone }, { 96, BlockType::Conglomerate }, { 74, BlockType::Sandstone },
    { 56, BlockType::Shale },      { 38, BlockType::Limestone },    { 22, BlockType::Dolomite },
    { 8,  BlockType::Chert },      { INT_MIN, BlockType::Granite }
};
const Bed BEDS_KARST[] = {
    { 100, BlockType::Chalk }, { 70, BlockType::Limestone }, { 48, BlockType::Dolomite },
    { 30, BlockType::Marble }, { 14, BlockType::Schist },    { INT_MIN, BlockType::Granite }
};
const Bed BEDS_BELT[] = {
    { 94, BlockType::Slate },     { 72, BlockType::Phyllite }, { 50, BlockType::Schist },
    { 30, BlockType::Quartzite }, { 14, BlockType::Gneiss },   { INT_MIN, BlockType::Granite }
};

struct Suite { const Bed* beds; int count; BlockType vein; const char* name; };

const Suite SUITES[] = {
    { BEDS_SHIELD,   6, BlockType::Quartzite, "Shield" },
    { BEDS_VOLCANIC, 6, BlockType::Rhyolite,  "Volcanic Province" },
    { BEDS_BASIN,    8, BlockType::Chert,     "Sedimentary Basin" },
    { BEDS_KARST,    6, BlockType::Marble,    "Carbonate Platform" },
    { BEDS_BELT,     6, BlockType::Quartzite, "Fold Belt" }
};

struct OreDef { BlockType ore; int yMin; int yMax; float rate; };

BlockType OrePebbleFor(BlockType ore) {
    switch (ore) {
    case BlockType::BituminousCoal:
    case BlockType::Lignite:        return BlockType::CoalPebble;
    case BlockType::Hematite:
    case BlockType::Magnetite:
    case BlockType::Limonite:       return BlockType::IronPebble;
    case BlockType::NativeCopper:
    case BlockType::Malachite:
    case BlockType::Tetrahedrite:   return BlockType::CopperPebble;
    case BlockType::Cassiterite:
    case BlockType::Bismuthinite:   return BlockType::TinPebble;
    case BlockType::Sphalerite:     return BlockType::ZincPebble;
    case BlockType::NativeGold:     return BlockType::GoldPebble;
    case BlockType::NativeSilver:   return BlockType::SilverPebble;
    case BlockType::Galena:         return BlockType::LeadPebble;
    case BlockType::Kimberlite:     return BlockType::DiamondPebble;
    case BlockType::Barite:         return BlockType::BaritePebble;
    case BlockType::Fluorite:       return BlockType::FluoritePebble;
    case BlockType::Phosphorite:    return BlockType::PhosphoritePebble;
    case BlockType::Sylvite:        return BlockType::PotashPebble;
    case BlockType::Wolframite:     return BlockType::TungstenPebble;
    case BlockType::Uraninite:      return BlockType::UraniumPebble;
    default:                         return BlockType::StonePebble;
    }
}

const OreDef ORE_CARBONATE[] = {
    { BlockType::Galena,         8, 58, 0.0060f },
    { BlockType::Sphalerite,    10, 62, 0.0060f },
    { BlockType::Limonite,      30, 78, 0.0045f },
    { BlockType::BituminousCoal, 18, 70, 0.0055f },
    { BlockType::Barite,        12, 64, 0.0045f },
    { BlockType::Fluorite,       6, 56, 0.0055f }
};
const OreDef ORE_SEDIMENTARY[] = {
    { BlockType::BituminousCoal, 14, 82, 0.0130f },
    { BlockType::Lignite,        52, 96, 0.0060f },
    { BlockType::Hematite,        8, 66, 0.0075f },
    { BlockType::Limonite,       34, 84, 0.0050f },
    { BlockType::Phosphorite,    40, 90, 0.0035f },
    { BlockType::Sylvite,        58, 98, 0.0075f }
};
const OreDef ORE_IGNEOUS[] = {
    { BlockType::Magnetite,       6, 62, 0.0080f },
    { BlockType::NativeCopper,   14, 70, 0.0070f },
    { BlockType::Malachite,      26, 74, 0.0055f },
    { BlockType::Bismuthinite,    4, 26, 0.0030f },
    { BlockType::Uraninite,       4, 40, 0.0028f }
};
const OreDef ORE_GRANITIC[] = {
    { BlockType::Cassiterite,     6, 54, 0.0075f },
    { BlockType::Tetrahedrite,   12, 60, 0.0050f },
    { BlockType::Bismuthinite,    4, 30, 0.0035f },
    { BlockType::NativeSilver,    5, 34, 0.0035f },
    { BlockType::Wolframite,      8, 52, 0.0055f }
};
const OreDef ORE_METAMORPHIC[] = {
    { BlockType::NativeGold,      5, 46, 0.0050f },
    { BlockType::NativeSilver,    8, 50, 0.0045f },
    { BlockType::Hematite,       10, 64, 0.0055f },
    { BlockType::Tetrahedrite,   14, 62, 0.0040f }
};

template <int N>
constexpr int CountOf(const OreDef(&)[N]) { return N; }

struct OreTable { const OreDef* defs; int count; };

OreTable HostOreTable(BlockType host) {
    if (WorldGenerator::IsCarbonate(host))
        return { ORE_CARBONATE, CountOf(ORE_CARBONATE) };
    if (WorldGenerator::IsSedimentary(host))
        return { ORE_SEDIMENTARY, CountOf(ORE_SEDIMENTARY) };
    if (host == BlockType::Granite || host == BlockType::Diorite ||
        host == BlockType::Rhyolite)
        return { ORE_GRANITIC, CountOf(ORE_GRANITIC) };
    if (WorldGenerator::IsIgneous(host))
        return { ORE_IGNEOUS, CountOf(ORE_IGNEOUS) };
    if (WorldGenerator::IsMetamorphic(host))
        return { ORE_METAMORPHIC, CountOf(ORE_METAMORPHIC) };
    return { ORE_SEDIMENTARY, CountOf(ORE_SEDIMENTARY) };
}

}

const char* WorldGenerator::RockSuiteName(RockSuite s) {
    int i = (int)s;
    if (i < 0 || i >= (int)RockSuite::Count) return "Unknown";
    return SUITES[i].name;
}

bool WorldGenerator::IsCarbonate(BlockType r) {
    return r == BlockType::Limestone || r == BlockType::Dolomite ||
        r == BlockType::Chalk || r == BlockType::Marble;
}

bool WorldGenerator::IsSedimentary(BlockType r) {
    return r == BlockType::Shale || r == BlockType::Sandstone ||
        r == BlockType::RedSandstone || r == BlockType::Claystone ||
        r == BlockType::Conglomerate || r == BlockType::Chert || IsCarbonate(r);
}

bool WorldGenerator::IsIgneous(BlockType r) {
    return r == BlockType::Granite || r == BlockType::Diorite || r == BlockType::Gabbro ||
        r == BlockType::Rhyolite || r == BlockType::Basalt ||
        r == BlockType::Andesite || r == BlockType::Dacite;
}

bool WorldGenerator::IsMetamorphic(BlockType r) {
    return r == BlockType::Quartzite || r == BlockType::Slate || r == BlockType::Phyllite ||
        r == BlockType::Schist || r == BlockType::Gneiss || r == BlockType::Marble;
}

BlockType WorldGenerator::BedRockAt(RockSuite suite, int y) {
    const Suite& s = SUITES[(int)suite];
    for (int i = 0; i < s.count; ++i)
        if (y >= s.beds[i].top) return s.beds[i].rock;
    return s.beds[s.count - 1].rock;
}

RockColumn WorldGenerator::ComputeRock(int x, int z) {
    RockColumn rc;

    float fx = (float)x, fz = (float)z;
    float wx = fx + noiseWarpX.GetNoise(fx, fz) * 140.0f;
    float wz = fz + noiseWarpZ.GetNoise(fx, fz) * 140.0f;

    float cell = noiseProvince.GetNoise(wx, wz) * 0.5f + 0.5f;
    int idx = (int)(cell * (float)RockSuite::Count);
    if (idx < 0) idx = 0;
    if (idx >= (int)RockSuite::Count) idx = (int)RockSuite::Count - 1;
    rc.suite = (RockSuite)idx;

    rc.bedOffset = noiseBed.GetNoise(fx, fz) * 13.0f +
        noiseBed.GetNoise(fx * 4.3f + 811.0f, fz * 4.3f - 233.0f) * 3.5f;

    float vein = noiseVein.GetNoise(fx, fz);
    if (vein > 0.68f) {
        float hv = Hash(x >> 3, z >> 3);
        rc.veinActive = true;
        rc.veinRock = SUITES[idx].vein;
        rc.veinBottom = 5 + (int)(hv * 12.0f);
        rc.veinTop = rc.veinBottom + 26 + (int)(Hash((x >> 3) + 61, (z >> 3) - 47) * 52.0f);
    }

    return rc;
}

RockColumn WorldGenerator::GetRockColumn(int x, int z) { return ComputeRock(x, z); }

BlockType WorldGenerator::GetSurfaceRock(int x, int z) {
    ColumnInfo col = GetColumnInfo(x, z);
    return GetStoneType(x, std::max(1, col.height - ROCK_PROBE_DEPTH), z, col);
}

BlockType WorldGenerator::GetStoneType(int x, int y, int z, const ColumnInfo& col) const {
    const RockColumn& rc = col.rock;

    if (rc.veinActive && y >= rc.veinBottom && y <= rc.veinTop)
        return rc.veinRock;

    int by = y + (int)rc.bedOffset;

    BlockType rock = BedRockAt(rc.suite, by);

    float mottle = Hash3(x >> 1, by >> 2, z >> 1);
    if (mottle > 0.955f) {
        BlockType neighbour = BedRockAt(rc.suite, by + ((mottle > 0.978f) ? 14 : -14));
        if (neighbour != rock) return neighbour;
    }

    bool nearSurface = (y > col.height - 14);

    if (nearSurface && (col.biome == BiomeType::Scorched ||
        col.biome == BiomeType::SaltFlat))
        return BlockType::RedSandstone;

    if (nearSurface && (col.biome == BiomeType::Desert ||
        col.biome == BiomeType::TemperateDesert ||
        col.biome == BiomeType::Shrubland) && IsSedimentary(rock))
        return BlockType::Sandstone;

    return rock;
}

BlockType WorldGenerator::GetOreBlock(int x, int y, int z, BlockType host,
    const RockColumn& rc) const {
    OreTable table = HostOreTable(host);
    const OreDef* defs = table.defs;

    bool inVein = rc.veinActive && y >= rc.veinBottom && y <= rc.veinTop;
    float boost = (inVein ? 3.2f : 1.0f) * std::clamp(gen.oreDensity, 0.05f, 6.0f);

    float h = Hash3(x, y, z);
    float acc = 0.0f;
    for (int i = 0; i < table.count; ++i) {
        if (y < defs[i].yMin || y > defs[i].yMax) continue;
        acc += defs[i].rate * boost;
        if (h > 1.0f - acc) return defs[i].ore;
    }

    if (rc.suite == RockSuite::Shield && y >= 1 && y <= 22 && h < 0.0012f)
        return BlockType::Kimberlite;

    return host;
}

BlockType WorldGenerator::PebbleForRock(BlockType rock) const {
    switch (rock) {
    case BlockType::Granite:
    case BlockType::Diorite:
    case BlockType::Gabbro:         return BlockType::GranitePebble;
    case BlockType::Basalt:
    case BlockType::Andesite:
    case BlockType::Rhyolite:
    case BlockType::Dacite:         return BlockType::BasaltPebble;
    case BlockType::Limestone:
    case BlockType::Dolomite:
    case BlockType::Chalk:
    case BlockType::Marble:         return BlockType::LimestonePebble;
    case BlockType::Sandstone:
    case BlockType::RedSandstone:
    case BlockType::Conglomerate:
    case BlockType::Claystone:      return BlockType::SandstonePebble;
    case BlockType::Chert:
    case BlockType::Shale:
    case BlockType::Slate:          return BlockType::FlintPebble;
    case BlockType::Gneiss:
    case BlockType::Schist:
    case BlockType::Phyllite:       return BlockType::GranitePebble;
    default:                         return BlockType::StonePebble;
    }
}

BlockType WorldGenerator::GetDecorationBlock(int x, int y, int z, const ColumnInfo& col) {
    const int        surfaceY = col.height;
    const BiomeType  biome = col.biome;
    const RiverInfo& river = col.river;

    if (y > surfaceY + 24 || surfaceY <= SEA_LEVEL) return BlockType::Air;

    if (river.channel) return BlockType::Air;
    if (river.active && y <= river.waterLevel) return BlockType::Air;

    if (CarvesAt(x, surfaceY, z, surfaceY, river)) return BlockType::Air;

    for (int dx = -3; dx <= 3; dx++) {
        for (int dz = -3; dz <= 3; dz++) {
            int tx = x + dx, tz = z + dz;
            float chance = Hash(tx, tz);
            if (chance <= 0.978f) continue;
            float sizeRng = Hash(tx, tz + 7);

            bool isTree = false, isConical = false;
            BlockType logT = BlockType::OakLog, leavesT = BlockType::OakLeaves;
            int trunkH = 4 + (int)(sizeRng * 2.0f);

            BiomeType tBiome = GetBiome(tx, tz);
            switch (tBiome) {
            case BiomeType::TemperateDeciduousForest:
            case BiomeType::TemperateRainforest:
                if (chance > 0.993f) { isTree = true; trunkH = 5 + (int)(sizeRng * 3); }
                break;
            case BiomeType::Taiga:
                if (chance > 0.988f) {
                    isTree = true; isConical = true;
                    logT = BlockType::SpruceLog; leavesT = BlockType::SpruceLeaves;
                    trunkH = 7 + (int)(sizeRng * 5);
                }
                break;
            case BiomeType::TropicalRainforest:
                if (chance > 0.980f) {
                    isTree = true;
                    logT = BlockType::JungleLog; leavesT = BlockType::JungleLeaves;
                    trunkH = 8 + (int)(sizeRng * 6);
                }
                break;
            case BiomeType::Savanna:
                if (chance > 0.996f) {
                    isTree = true;
                    logT = BlockType::AcaciaLog; leavesT = BlockType::AcaciaLeaves;
                    trunkH = 4 + (int)(sizeRng * 2);
                }
                break;
            case BiomeType::Tundra:
                if (chance > 0.998f) {
                    isTree = true; isConical = true;
                    logT = BlockType::SpruceLog; leavesT = BlockType::SpruceLeaves;
                    trunkH = 3 + (int)(sizeRng * 2);
                }
                break;
            default: break;
            }

            if (!isTree) continue;

            float tb = GetBaseHeight(tx, tz);
            RiverInfo tr = ComputeRiver(tx, tz, tb);
            int baseY = (int)tr.terrainH;
            if (tr.channel || baseY <= SEA_LEVEL) continue;
            if (IsCaveAt(tx, baseY, tz, baseY)) continue;

            int treeTop = baseY + trunkH;
            if (y <= baseY || y > treeTop) continue;

            if (dx == 0 && dz == 0 && y < treeTop) return logT;

            if (isConical) {
                int layer = treeTop - y;
                if (layer >= 0 && layer <= trunkH - 2) {
                    int r = (layer % 2 == 0) ? 1 : 2;
                    if (layer == 0) r = 1;
                    if (std::abs(dx) <= r && std::abs(dz) <= r) {
                        if (std::abs(dx) == r && std::abs(dz) == r && r > 1 && Hash(tx + y, tz - y) > 0.5f) return BlockType::Air;
                        if (!(dx == 0 && dz == 0 && y < treeTop)) return leavesT;
                    }
                }
            }
            else {
                if (y >= treeTop - 2 && y <= treeTop) {
                    int r = (y == treeTop) ? 1 : 2;
                    if (std::abs(dx) <= r && std::abs(dz) <= r) {
                        if (std::abs(dx) == r && std::abs(dz) == r) {
                            if (y == treeTop || Hash(tx + y, tz) > 0.4f) return BlockType::Air;
                        }

                        if (!(dx == 0 && dz == 0 && y < treeTop)) return leavesT;
                    }
                }
            }
        }
    }

    if (y != surfaceY + 1) return BlockType::Air;

    {
        BlockType ground = ResolveSurface(x, surfaceY, z, col);
        bool soil = (ground == BlockType::Grass || ground == BlockType::Dirt ||
            ground == BlockType::CoarseDirt || ground == BlockType::Mud ||
            ground == BlockType::Peat || ground == BlockType::Sand ||
            ground == BlockType::RedSand);
        if (!soil) return BlockType::Air;
    }

    float dcR = Hash(x + 91, z - 44);
    if (river.active && !river.channel && surfaceY <= river.waterLevel + 2 && dcR > 0.84f)
        return BlockType::Reed;
    if (biome == BiomeType::Beach && surfaceY <= SEA_LEVEL + 2 && dcR > 0.93f)
        return BlockType::Reed;

    float lushN = (noiseDetail.GetNoise((float)x * 2.3f, (float)z * 2.3f) + 1.0f) * 0.5f;
    float lush = lushN * lushN;
    float dcG = Hash(x, z + 500);
    float dcF = Hash(x + 300, z + 700);
    float dcS = Hash(x + 811, z + 271);
    float dcB = Hash(x - 617, z + 1279);

    bool berryBiome = (biome == BiomeType::Taiga ||
        biome == BiomeType::TemperateDeciduousForest ||
        biome == BiomeType::TemperateRainforest);
    if (berryBiome && surfaceY > SEA_LEVEL + 1) {
        float patch = (noiseDetail.GetNoise((float)x * 0.55f + 4400.0f,
            (float)z * 0.55f - 2100.0f) + 1.0f) * 0.5f;
        if (patch > 0.63f && dcB > 0.962f - (patch - 0.63f) * 0.55f) {
            BlockType ground = ResolveSurface(x, surfaceY, z, col);
            if (ground == BlockType::Grass || ground == BlockType::Dirt ||
                ground == BlockType::CoarseDirt)
                return (dcB > 0.986f) ? BlockType::BerryBushRipe : BlockType::BerryBush;
        }
    }

    switch (biome) {
    case BiomeType::Desert: case BiomeType::Scorched:
        if (dcG > 0.998f - lush * 0.006f) return BlockType::Cactus;
        if (dcF > 0.990f - lush * 0.012f) return BlockType::DeadBush;
        break;
    case BiomeType::Taiga:
        if (dcS > 0.992f - lush * 0.012f) return BlockType::BrownMushroom;
        if (dcG > 1.0f - 0.10f * (0.06f + lush)) return BlockType::TallGrass;
        break;
    case BiomeType::Swamp:
        if (dcG > 1.0f - 0.26f * (0.10f + lush)) return BlockType::TallGrass;
        if (dcS > 0.972f) return BlockType::BrownMushroom;
        if (dcF > 0.984f) return BlockType::Reed;
        break;
    case BiomeType::Bayou:
        if (surfaceY <= SEA_LEVEL + 2 && dcF > 0.955f) return BlockType::Cattail;
        if (dcG > 1.0f - 0.30f * (0.12f + lush)) return BlockType::Fern;
        if (dcS > 0.968f) return BlockType::Toadstool;
        break;
    case BiomeType::Floodplain:
        if (surfaceY <= SEA_LEVEL + 1 && dcF > 0.930f) return BlockType::LilyPad;
        if (dcG > 1.0f - 0.34f * (0.14f + lush)) return BlockType::TallGrass;
        if (dcS > 0.962f) return BlockType::Cattail;
        break;
    case BiomeType::Bog:
        if (dcG > 1.0f - 0.22f * (0.08f + lush)) return BlockType::CranberryBush;
        if (dcS > 0.966f) return BlockType::Toadstool;
        if (dcF > 0.980f) return BlockType::Reed;
        break;
    case BiomeType::ConiferousForest:
        if (dcG > 1.0f - 0.20f * (0.07f + lush)) return BlockType::TallGrass;
        if (dcS > 0.958f) return BlockType::Toadstool;
        if (lush > 0.40f && dcF > 0.986f) return BlockType::Dandelion;
        break;
    case BiomeType::Highland:
        if (dcG > 1.0f - 0.30f * (0.10f + lush)) return BlockType::TallGrass;
        if (dcF > 0.948f) return BlockType::Clover;
        break;
    case BiomeType::Beach:
        if (dcF > 0.972f) return BlockType::DuneGrass;
        break;
    case BiomeType::Mangrove:
        if (dcG > 1.0f - 0.20f * (0.08f + lush)) return BlockType::Reed;
        if (dcS > 0.986f) return BlockType::Fern;
        break;
    case BiomeType::BambooForest:
        if (dcG > 1.0f - 0.30f * (0.12f + lush)) return BlockType::Reed;
        if (dcF > 0.980f) return BlockType::Fern;
        break;
    case BiomeType::Moorland:
        if (dcG > 1.0f - 0.16f * (0.06f + lush)) return BlockType::TallGrass;
        if (dcS > 0.988f) return BlockType::BrownMushroom;
        break;
    case BiomeType::Steppe:
        if (dcG > 1.0f - 0.18f * (0.05f + lush)) return BlockType::TallGrass;
        if (dcF > 0.995f) return BlockType::Dandelion;
        break;
    case BiomeType::Shrubland:
        if (dcF > 0.976f - lush * 0.010f) return BlockType::DeadBush;
        if (dcG > 1.0f - 0.08f * (0.04f + lush)) return BlockType::TallGrass;
        break;
    case BiomeType::BirchForest:
        if (dcG > 1.0f - 0.22f * (0.06f + lush)) return BlockType::TallGrass;
        if (lush > 0.40f && dcF > 0.980f) return BlockType::Dandelion;
        if (dcS > 0.994f) return BlockType::RedMushroom;
        break;
    case BiomeType::RedwoodForest:
        if (dcG > 1.0f - 0.18f * (0.06f + lush)) return BlockType::Fern;
        if (dcS > 0.988f) return BlockType::BrownMushroom;
        break;
    case BiomeType::AlpineMeadow:
        if (dcG > 1.0f - 0.24f * (0.08f + lush)) return BlockType::TallGrass;
        if (dcF > 0.966f) return (dcF > 0.984f) ? BlockType::Rose : BlockType::Dandelion;
        break;
    case BiomeType::ColdDesert:
        if (dcF > 0.9975f) return BlockType::DeadBush;
        break;
    case BiomeType::VolcanicWastes:
    case BiomeType::SaltFlat:
    case BiomeType::Glacier:
        break;
    case BiomeType::Tundra:
        if (dcG > 1.0f - 0.06f * (0.05f + lush)) return BlockType::TallGrass;
        if (dcS > 0.997f) return BlockType::BrownMushroom;
        break;
    case BiomeType::SnowyTundra: case BiomeType::IceSpikes:
        if (dcF > 0.9985f) return BlockType::DeadBush;
        break;
    case BiomeType::Mountains:
        if (surfaceY < 130) {
            if (dcG > 1.0f - 0.14f * (0.08f + lush)) return BlockType::TallGrass;
            if (lush > 0.40f && dcF > 0.985f) return BlockType::Dandelion;
            if (dcS > 0.992f) return BlockType::BrownMushroom;
        }
        break;
    case BiomeType::TropicalRainforest:
        if (dcG > 1.0f - 0.22f * (0.10f + lush)) return BlockType::TallGrass;
        if (lush > 0.25f && dcS > 0.92f && dcS < 0.975f) return BlockType::Fern;
        if (lush > 0.35f && dcF > 0.975f) return (dcF > 0.988f) ? BlockType::Rose : BlockType::Dandelion;
        if (dcS > 0.990f) return BlockType::BrownMushroom;
        break;
    case BiomeType::TemperateRainforest:
        if (dcG > 1.0f - 0.22f * (0.06f + lush)) return BlockType::TallGrass;
        if (lush > 0.30f && dcS > 0.94f && dcS < 0.985f) return BlockType::Fern;
        if (lush > 0.45f && dcF > 0.980f) return BlockType::Rose;
        if (dcS > 0.996f) return BlockType::BrownMushroom;
        break;
    default:
        if (dcG > 1.0f - 0.22f * (0.05f + lush)) return BlockType::TallGrass;
        if (lush > 0.55f && dcS > 0.975f && dcS < 0.992f) return BlockType::Fern;
        if (lush > 0.45f && dcF > 0.978f - lush * 0.012f)
            return (dcF > 0.991f) ? BlockType::Dandelion : BlockType::Rose;
        if (dcS > 0.9975f) return BlockType::BrownMushroom;
        if (lush > 0.50f && dcF < 0.0015f) return BlockType::Pumpkin;
        break;
    }

    if (y == surfaceY + 1) {
        float pebChance = Hash(x ^ 0xABCD, z ^ 0x1234);
        bool isBeach = (biome == BiomeType::Beach || biome == BiomeType::Desert || biome == BiomeType::TemperateDesert);

        float threshold = isBeach ? 0.992f : 0.997f;
        if (biome == BiomeType::Mountains) threshold = 0.990f;
        if (biome == BiomeType::Tundra || biome == BiomeType::SnowyTundra) threshold = 0.996f;

        if (pebChance > threshold) {
            int hostY = std::max(1, surfaceY - ROCK_PROBE_DEPTH);
            BlockType host = GetStoneType(x, hostY, z, col);

            float oreHash = Hash3(x, surfaceY - 15, z);
            if (oreHash > 0.74f) {
                int oreY = std::clamp(surfaceY - 30 - (int)(Hash(x - 331, z + 733) * 26.0f),
                    4, std::max(5, surfaceY - 8));
                BlockType deep = GetStoneType(x, oreY, z, col);

                if (col.rock.suite == RockSuite::Shield && oreHash > 0.9975f)
                    return BlockType::DiamondPebble;

                OreTable table = HostOreTable(deep);
                const OreDef* defs = table.defs;
                float total = 0.0f;
                for (int i = 0; i < table.count; ++i)
                    if (oreY >= defs[i].yMin - 12 && oreY <= defs[i].yMax + 12)
                        total += defs[i].rate;

                if (total > 0.0f) {
                    float pick = Hash(x + 5501, z - 3307) * total;
                    float acc = 0.0f;
                    for (int i = 0; i < table.count; ++i) {
                        if (oreY < defs[i].yMin - 12 || oreY > defs[i].yMax + 12) continue;
                        acc += defs[i].rate;
                        if (pick <= acc) return OrePebbleFor(defs[i].ore);
                    }
                }
            }

            return PebbleForRock(host);
        }
    }

    return BlockType::Air;
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
