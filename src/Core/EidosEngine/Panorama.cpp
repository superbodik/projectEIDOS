#include "../EidosEngine.h"
#include <unordered_map>
#include "../CommandManager.h"
#include "../../Cinematic/Trailer.h"
#include "../../Cinematic/AutoShot.h"
#include "../../Inventory/BlockInfo.h"
#include "../../Inventory/MiningRules.h"
#include "../../Progression/QuestSystem.h"
#include "../../World/Chunk.h"
#include "rlgl.h"
#include <raymath.h>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <deque>
#include <chrono>

namespace fs = std::filesystem;

static float PanoHash(int a, int b) {
    unsigned int h = (unsigned int)a * 374761393u + (unsigned int)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFFu) / 65535.0f;
}

static float PanoRidge(float az, float seedOff, float scale) {
    float v = 0.0f;
    v += sinf(az * 2.0f + seedOff) * 0.45f;
    v += sinf(az * 5.0f + seedOff * 1.7f) * 0.26f;
    v += sinf(az * 11.0f + seedOff * 2.3f) * 0.16f;
    v += sinf(az * 23.0f + seedOff * 3.1f) * 0.08f;
    return v * scale;
}

void EidosEngine::LoadPanorama() {
    for (int i = 0; i < 6; i++) {
        if (panoramaTextures[i].id != 0) {
            UnloadTexture(panoramaTextures[i]);
            panoramaTextures[i] = { 0 };
        }
    }

    const int S = 512;
    const Vector3 sunDir = Vector3Normalize({ 0.62f, 0.20f, -0.76f });

    auto faceDir = [](int f, float u, float v) -> Vector3 {
        switch (f) {
        case 0:  return Vector3Normalize({ 1.0f, 1.0f - 2.0f * v, -1.0f + 2.0f * u });
        case 1:  return Vector3Normalize({ -1.0f, 1.0f - 2.0f * v, 1.0f - 2.0f * u });
        case 2:  return Vector3Normalize({ 1.0f - 2.0f * u, 1.0f, -1.0f + 2.0f * v });
        case 3:  return Vector3Normalize({ 1.0f - 2.0f * u, -1.0f, 1.0f - 2.0f * v });
        case 4:  return Vector3Normalize({ 1.0f - 2.0f * u, 1.0f - 2.0f * v, 1.0f });
        default: return Vector3Normalize({ -1.0f + 2.0f * u, 1.0f - 2.0f * v, -1.0f });
        }
        };

    for (int f = 0; f < 6; f++) {
        Image img = GenImageColor(S, S, BLACK);

        for (int py = 0; py < S; py++) {
            for (int px = 0; px < S; px++) {
                Vector3 d = faceDir(f, ((float)px + 0.5f) / S, ((float)py + 0.5f) / S);
                float el = d.y;
                float az = atan2f(d.z, d.x);

                float t = std::clamp(el * 1.4f + 0.12f, 0.0f, 1.0f);
                Color zenith = { 26, 52, 104, 255 };
                Color mid = { 74, 126, 186, 255 };
                Color horizon = { 214, 168, 132, 255 };
                float r, g, b;
                if (t < 0.42f) {
                    float k = t / 0.42f;
                    r = horizon.r + (mid.r - horizon.r) * k;
                    g = horizon.g + (mid.g - horizon.g) * k;
                    b = horizon.b + (mid.b - horizon.b) * k;
                }
                else {
                    float k = (t - 0.42f) / 0.58f;
                    r = mid.r + (zenith.r - mid.r) * k;
                    g = mid.g + (zenith.g - mid.g) * k;
                    b = mid.b + (zenith.b - mid.b) * k;
                }

                float sunDot = d.x * sunDir.x + d.y * sunDir.y + d.z * sunDir.z;
                if (sunDot > 0.0f) {
                    float glow = powf(sunDot, 42.0f) * 0.85f + powf(sunDot, 6.0f) * 0.22f;
                    r += 255.0f * glow; g += 232.0f * glow; b += 176.0f * glow;
                }

                if (el > 0.22f) {
                    float sx = d.x / (fabsf(d.y) + 0.001f);
                    float sz = d.z / (fabsf(d.y) + 0.001f);
                    int gx = (int)floorf(sx * 46.0f), gz = (int)floorf(sz * 46.0f);
                    float sp = PanoHash(gx, gz);
                    if (sp > 0.9955f) {
                        float tw = 0.55f + PanoHash(gz, gx) * 0.45f;
                        float fade = std::clamp((el - 0.22f) * 2.6f, 0.0f, 1.0f);
                        float a = tw * fade * 190.0f;
                        r += a; g += a; b += a * 0.94f;
                    }
                }

                struct Layer { float base, scale, dist; Color tone; };
                const Layer layers[4] = {
                    { 0.175f, 0.062f, 0.80f, { 178, 194, 216, 255 } },
                    { 0.132f, 0.082f, 0.62f, { 140, 158, 186, 255 } },
                    { 0.088f, 0.104f, 0.40f, {  98, 114, 142, 255 } },
                    { 0.038f, 0.128f, 0.16f, {  64,  76,  98, 255 } },
                };

                float sunAz = atan2f(sunDir.z, sunDir.x);

                for (int L = 0; L < 4; L++) {
                    float ridge = layers[L].base + PanoRidge(az, 1.3f * (float)(L + 1), layers[L].scale);
                    if (el >= ridge) continue;

                    float depth = std::clamp((ridge - el) * 6.0f, 0.0f, 1.0f);
                    Color tn = layers[L].tone;

                    float facing = cosf(az - sunAz);
                    float lit = 0.80f + 0.38f * std::clamp(facing, -1.0f, 1.0f);
                    float tr = tn.r * lit, tg = tn.g * lit * 0.99f, tb = tn.b * lit * 0.96f;
                    if (facing > 0.0f) {
                        float warm = facing * facing * 0.30f;
                        tr += 46.0f * warm; tg += 26.0f * warm; tb -= 6.0f * warm;
                    }

                    if (ridge > 0.100f && el > ridge - 0.030f - PanoRidge(az, 7.7f, 0.012f)) {
                        float cap = std::clamp((el - (ridge - 0.034f)) * 32.0f, 0.0f, 1.0f);
                        tr += (236.0f - tr) * cap; tg += (242.0f - tg) * cap; tb += (250.0f - tb) * cap;
                    }

                    tr *= (1.0f - depth * 0.22f);
                    tg *= (1.0f - depth * 0.22f);
                    tb *= (1.0f - depth * 0.20f);

                    float haze = 1.0f - layers[L].dist * (1.0f - depth * 0.5f);
                    r = tr + (r - tr) * (1.0f - haze);
                    g = tg + (g - tg) * (1.0f - haze);
                    b = tb + (b - tb) * (1.0f - haze);
                }

                if (el < -0.05f) {
                    float k = std::clamp((-el - 0.05f) * 2.6f, 0.0f, 1.0f);
                    r += (30.0f - r) * k; g += (36.0f - g) * k; b += (48.0f - b) * k;
                }

                ImageDrawPixel(&img, px, py, {
                    (unsigned char)std::clamp(r, 0.0f, 255.0f),
                    (unsigned char)std::clamp(g, 0.0f, 255.0f),
                    (unsigned char)std::clamp(b, 0.0f, 255.0f), 255 });
            }
        }

        panoramaTextures[f] = LoadTextureFromImage(img);
        SetTextureFilter(panoramaTextures[f], TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(panoramaTextures[f], TEXTURE_WRAP_CLAMP);
        UnloadImage(img);
    }

    hasPanorama = true;
}

void EidosEngine::UnloadPanorama() {
    for (int i = 0; i < 6; i++) {
        if (panoramaTextures[i].id != 0) {
            UnloadTexture(panoramaTextures[i]);
            panoramaTextures[i] = { 0 };
        }
    }
    hasPanorama = false;
}

void EidosEngine::DrawPanorama() {
    if (!hasPanorama) return;

    rlDisableDepthMask();
    rlDisableDepthTest();
    rlDisableBackfaceCulling();

    Vector3 p = player.camera.position;
    float s = 50.0f;

    auto drawFace = [](Texture2D tex, Vector3 tl, Vector3 bl, Vector3 br, Vector3 tr) {
        if (tex.id == 0) return;
        rlSetTexture(tex.id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(tl.x, tl.y, tl.z);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(tr.x, tr.y, tr.z);
        rlEnd();
        rlSetTexture(0);
        };

    drawFace(panoramaTextures[0], { p.x + s, p.y + s, p.z - s }, { p.x + s, p.y - s, p.z - s }, { p.x + s, p.y - s, p.z + s }, { p.x + s, p.y + s, p.z + s });
    drawFace(panoramaTextures[1], { p.x - s, p.y + s, p.z + s }, { p.x - s, p.y - s, p.z + s }, { p.x - s, p.y - s, p.z - s }, { p.x - s, p.y + s, p.z - s });
    drawFace(panoramaTextures[2], { p.x + s, p.y + s, p.z - s }, { p.x + s, p.y + s, p.z + s }, { p.x - s, p.y + s, p.z + s }, { p.x - s, p.y + s, p.z - s });
    drawFace(panoramaTextures[3], { p.x + s, p.y - s, p.z + s }, { p.x + s, p.y - s, p.z - s }, { p.x - s, p.y - s, p.z - s }, { p.x - s, p.y - s, p.z + s });
    drawFace(panoramaTextures[4], { p.x + s, p.y + s, p.z + s }, { p.x + s, p.y - s, p.z + s }, { p.x - s, p.y - s, p.z + s }, { p.x - s, p.y + s, p.z + s });
    drawFace(panoramaTextures[5], { p.x - s, p.y + s, p.z - s }, { p.x - s, p.y - s, p.z - s }, { p.x + s, p.y - s, p.z - s }, { p.x + s, p.y + s, p.z - s });

    rlEnableBackfaceCulling();
    rlEnableDepthTest();
    rlEnableDepthMask();
}

