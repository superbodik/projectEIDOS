#include "SkySystem.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

SkySystem::SkySystem() {
    timeOfDay = 0.50f;
    InitClouds();
}

void SkySystem::InitClouds() {
    clouds.clear();

    for (int i = 0; i < 40; i++) {
        Cloud c;
        c.x = (float)(rand() % 800 - 400) + (rand() % 100) / 100.0f;
        c.z = (float)(rand() % 800 - 400) + (rand() % 100) / 100.0f;
        c.y = 140.0f + (float)(rand() % 60);
        c.size = 15.0f + (float)(rand() % 40);
        clouds.push_back(c);
    }
}

void SkySystem::Update(float dt, Vector3 playerPos) {
    centerPos = playerPos;

    timeOfDay += timeSpeed * dt * 0.05f;
    if (timeOfDay > 1.0f) timeOfDay -= 1.0f;

    CalculateCelestialPositions();
    CalculateColors();

    for (auto& c : clouds) {
        float dx = c.x - playerPos.x;
        float dz = c.z - playerPos.z;
        if (fabsf(dx) > 400.0f) c.x = playerPos.x + (dx > 0 ? -380.0f : 380.0f);
        if (fabsf(dz) > 400.0f) c.z = playerPos.z + (dz > 0 ? -380.0f : 380.0f);
    }
}

void SkySystem::CalculateCelestialPositions() {
    float angle = timeOfDay * 2.0f * PI;
    float dist = 400.0f;

    sunPosition.x = centerPos.x + cosf(angle - PI * 0.5f) * dist;
    sunPosition.y = centerPos.y + sinf(angle - PI * 0.5f) * dist;
    sunPosition.z = centerPos.z + sinf(angle) * dist * 0.35f;

    moonPosition.x = centerPos.x + cosf(angle + PI * 0.5f) * dist;
    moonPosition.y = centerPos.y + sinf(angle + PI * 0.5f) * dist;
    moonPosition.z = centerPos.z + sinf(angle + PI) * dist * 0.35f;
}

void SkySystem::CalculateColors() {
    float t = timeOfDay;

    Color nightTop = { 4,  4, 18, 255 };
    Color nightBot = { 7,  7, 25, 255 };
    Color preDawnTop = { 30, 15, 55, 255 };
    Color preDawnBot = { 90, 40, 70, 255 };
    Color sunriseTop = { 60, 50,120, 255 };
    Color sunriseBot = { 240,120, 50, 255 };
    Color dayTop = { 28,135,225, 255 };
    Color dayBot = { 155,208,255, 255 };
    Color sunsetTop = { 55, 40,105, 255 };
    Color sunsetBot = { 255,100, 35, 255 };
    Color duskTop = { 20, 12, 40, 255 };
    Color duskBot = { 60, 30, 50, 255 };

    if (t < 0.18f) {
        currentSkyColor = nightTop;
        currentFogColor = nightBot;
    }
    else if (t < 0.25f) {
        float b = (t - 0.18f) / 0.07f;
        currentSkyColor = LerpColor(nightTop, preDawnTop, b);
        currentFogColor = LerpColor(nightBot, preDawnBot, b);
    }
    else if (t < 0.33f) {
        float b = (t - 0.25f) / 0.08f;
        currentSkyColor = LerpColor(preDawnTop, sunriseTop, b);
        currentFogColor = LerpColor(preDawnBot, sunriseBot, b);
    }
    else if (t < 0.40f) {
        float b = (t - 0.33f) / 0.07f;
        currentSkyColor = LerpColor(sunriseTop, dayTop, b);
        currentFogColor = LerpColor(sunriseBot, dayBot, b);
    }
    else if (t < 0.65f) {
        currentSkyColor = dayTop;
        currentFogColor = dayBot;
    }
    else if (t < 0.73f) {
        float b = (t - 0.65f) / 0.08f;
        currentSkyColor = LerpColor(dayTop, sunsetTop, b);
        currentFogColor = LerpColor(dayBot, sunsetBot, b);
    }
    else if (t < 0.82f) {
        float b = (t - 0.73f) / 0.09f;
        currentSkyColor = LerpColor(sunsetTop, duskTop, b);
        currentFogColor = LerpColor(sunsetBot, duskBot, b);
    }
    else if (t < 0.90f) {
        float b = (t - 0.82f) / 0.08f;
        currentSkyColor = LerpColor(duskTop, nightTop, b);
        currentFogColor = LerpColor(duskBot, nightBot, b);
    }
    else {
        currentSkyColor = nightTop;
        currentFogColor = nightBot;
    }
}

Color SkySystem::LerpColor(Color a, Color b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}

void SkySystem::EnsureSunTexture() {
    if (sunRayTex.id != 0) return;

    const int S = 256;
    Image img = GenImageColor(S, S, { 0, 0, 0, 0 });

    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            float dx = x - 127.5f, dy = y - 127.5f;
            float d = sqrtf(dx * dx + dy * dy) / 127.5f;
            if (d > 1.0f) continue;

            float ang = atan2f(dy, dx);
            float rays = powf(fabsf(sinf(ang * 6.0f)), 3.0f) * 0.55f +
                powf(fabsf(sinf(ang * 11.0f + 1.3f)), 5.0f) * 0.45f;

            float core = expf(-d * d * 10.0f);
            float halo = expf(-d * 3.4f) * 0.55f;
            float beam = expf(-d * 2.2f) * rays * (1.0f - d);

            float a = std::clamp(core + halo + beam, 0.0f, 1.0f);
            ImageDrawPixel(&img, x, y, { 255, 244, 214, (unsigned char)(a * 255.0f) });
        }
    }

    sunRayTex = LoadTextureFromImage(img);
    SetTextureFilter(sunRayTex, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
}

void SkySystem::Draw(const Camera3D& cam) {
    rlDisableDepthMask();

    float sunH = std::clamp(sunPosition.y / 400.0f, 0.0f, 1.0f);
    if (sunH > 0.02f) {
        EnsureSunTexture();

        BeginBlendMode(BLEND_ADDITIVE);
        unsigned char rayA = (unsigned char)(70 + sunH * 130.0f);
        DrawBillboard(cam, sunRayTex, sunPosition, 420.0f, Color{ 255, 255, 255, rayA });
        DrawBillboard(cam, sunRayTex, sunPosition, 160.0f, Color{ 255, 255, 255, rayA });
        EndBlendMode();

        DrawSphere(sunPosition, 26.0f, Color{ 255, 252, 235, 255 });
    }

    float moonH = std::clamp(moonPosition.y / 400.0f, 0.0f, 1.0f);
    if (moonH > 0.02f) {
        EnsureSunTexture();
        BeginBlendMode(BLEND_ADDITIVE);
        unsigned char ma = (unsigned char)(45 + moonH * 70.0f);
        DrawBillboard(cam, sunRayTex, moonPosition, 210.0f, Color{ 168, 190, 236, ma });
        EndBlendMode();
        DrawSphere(moonPosition, 20.0f, Color{ 218, 222, 240, 255 });
    }

    rlEnableDepthMask();
}

void SkySystem::DrawClouds(Vector3 windDir, float windSpeed) {

    float sunH = std::clamp(sunPosition.y / 400.0f, 0.0f, 1.0f);
    if (sunH < 0.05f) return;

    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);

    unsigned char alpha = (unsigned char)(120 + sunH * 70.0f);
    Color cloudColor = { 255, 255, 255, alpha };
    Color cloudShade = { 216, 224, 238, (unsigned char)(alpha * 0.8f) };

    float dt = GetFrameTime();
    for (auto& c : clouds) {
        c.x += windDir.x * windSpeed * 0.3f * dt;
        c.z += windDir.z * windSpeed * 0.3f * dt;

        int seed = (int)(c.size * 97.0f);
        for (int p = 0; p < 4; p++) {
            float ox = (float)(((p * 73 + seed) % 17) - 8) * c.size * 0.055f;
            float oz = (float)(((p * 131 + seed) % 15) - 7) * c.size * 0.065f;
            float oy = (float)(((p * 37 + seed) % 5) - 2) * 1.3f;
            float w = c.size * (0.5f + 0.5f * (float)((p * 53 + seed) % 10) / 10.0f);
            Vector3 pos = { c.x + ox, c.y + oy, c.z + oz };
            DrawCube(pos, w, 2.4f + (float)p * 0.9f, w * 0.7f, (p % 2 == 0) ? cloudColor : cloudShade);
        }
    }

    EndBlendMode();
    rlEnableDepthMask();
}

void SkySystem::DrawMotes(Vector3 playerPos, Vector3 windDir, float windSpeed) {
    float sunH = std::clamp(sunPosition.y / 400.0f, 0.0f, 1.0f);
    if (sunH < 0.10f) return;

    if (motes.empty()) {
        motes.resize(70);
        for (size_t i = 0; i < motes.size(); i++) {
            float a = (float)(i * 2654435761u % 6283u) / 1000.0f;
            float r = 4.0f + (float)(i * 40503u % 26000u) / 1000.0f;
            motes[i].pos = { playerPos.x + cosf(a) * r,
                             playerPos.y - 4.0f + (float)(i * 9781u % 16000u) / 1000.0f,
                             playerPos.z + sinf(a) * r };
            motes[i].phase = a * 2.7f;
        }
    }

    float dt = GetFrameTime();
    float t = (float)GetTime();

    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    unsigned char a8 = (unsigned char)(90.0f * sunH);
    for (auto& m : motes) {
        m.pos.x += windDir.x * windSpeed * 0.06f * dt;
        m.pos.z += windDir.z * windSpeed * 0.06f * dt;
        m.pos.y += sinf(t * 0.8f + m.phase) * 0.12f * dt;

        if (m.pos.x - playerPos.x > 30.0f) m.pos.x -= 60.0f;
        if (playerPos.x - m.pos.x > 30.0f) m.pos.x += 60.0f;
        if (m.pos.z - playerPos.z > 30.0f) m.pos.z -= 60.0f;
        if (playerPos.z - m.pos.z > 30.0f) m.pos.z += 60.0f;
        if (m.pos.y - playerPos.y > 14.0f) m.pos.y -= 20.0f;
        if (playerPos.y - m.pos.y > 10.0f) m.pos.y += 20.0f;

        float s = 0.05f + 0.03f * sinf(t * 1.7f + m.phase * 3.1f);
        DrawCubeV(m.pos, { s, s, s }, { 255, 250, 225, a8 });
    }

    EndBlendMode();
    rlEnableDepthMask();
}

Color SkySystem::GetFogColor() const { return currentFogColor; }
Color SkySystem::GetSkyColor() const { return currentSkyColor; }
