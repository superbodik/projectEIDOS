#include "SkySystem.h"
#include "rlgl.h"

SkySystem::SkySystem() {
    timeOfDay = 0.25f;
}

void SkySystem::Update(float dt, Vector3 playerPos) {
    centerPos = playerPos;

    timeOfDay += timeSpeed * dt * 0.05f;
    if (timeOfDay > 1.0f) timeOfDay -= 1.0f;

    CalculateCelestialPositions();
    CalculateColors();
}

void SkySystem::CalculateCelestialPositions() {
    float angle = timeOfDay * 2.0f * PI;
    float dist = 400.0f;

    sunPosition.x = centerPos.x + cosf(angle + PI / 2.0f) * dist;
    sunPosition.y = centerPos.y + sinf(angle + PI / 2.0f) * dist;
    sunPosition.z = centerPos.z;

    moonPosition.x = centerPos.x + cosf(angle - PI / 2.0f) * dist;
    moonPosition.y = centerPos.y + sinf(angle - PI / 2.0f) * dist;
    moonPosition.z = centerPos.z;
}

void SkySystem::CalculateColors() {
    float t = timeOfDay;

    if (t < 0.20f) {
        currentSkyColor = dayTop;
        currentFogColor = dayBottom;
    }
    else if (t < 0.30f) {
        float blend = (t - 0.20f) * 10.0f;
        currentSkyColor = LerpColor(dayTop, sunsetTop, blend);
        currentFogColor = LerpColor(dayBottom, sunsetBottom, blend);
    }
    else if (t < 0.70f) {
        float blend = 0.0f;
        if (t < 0.4f) blend = (t - 0.30f) * 10.0f;
        else blend = 1.0f;

        currentSkyColor = LerpColor(sunsetTop, nightTop, blend);
        currentFogColor = LerpColor(sunsetBottom, nightBottom, blend);
    }
    else if (t < 0.80f) {
        float blend = (t - 0.70f) * 10.0f;
        currentSkyColor = LerpColor(nightTop, dayTop, blend);
        currentFogColor = LerpColor(nightBottom, dayBottom, blend);
    }
    else {
        currentSkyColor = dayTop;
        currentFogColor = dayBottom;
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

void SkySystem::Draw() {
    rlDisableDepthMask();

    DrawSphere(sunPosition, 30.0f, YELLOW);
    DrawSphere(moonPosition, 20.0f, LIGHTGRAY);

    rlEnableDepthMask();
}

Color SkySystem::GetFogColor() const {
    return currentFogColor;
}

Color SkySystem::GetSkyColor() const {
    return currentSkyColor;
}