#include "../WorldGenerator.h"
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

