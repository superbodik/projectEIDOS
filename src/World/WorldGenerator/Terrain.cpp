#include "../WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <climits>

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

