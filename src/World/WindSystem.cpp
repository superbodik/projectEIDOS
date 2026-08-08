#include "WindSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <sstream>
#include <iomanip>

WindSystem::WindSystem() {
    direction = { 0.7f, 0.0f, -0.7f };
    speed = 3.0f;
    gust = 0.0f;
    gustTimer = 0.0f;
    currentDir = 2.3f;
    targetDir = currentDir;
}

void WindSystem::Update(float dt, int biomeId, float weatherIntensity) {

    struct BiomeWind { float min, max; };
    static const BiomeWind biomeWinds[] = {
        { 6.0f, 14.0f },
        { 4.0f, 10.0f },
        { 2.0f, 7.0f },
        { 3.0f, 9.0f },
        { 2.0f, 6.0f },
        { 1.0f, 4.0f },
        { 3.0f, 8.0f },
        { 1.5f, 5.0f },
        { 2.0f, 6.0f },
        { 2.0f, 7.0f },
        { 3.0f, 9.0f },
        { 4.0f, 12.0f },
        { 5.0f, 14.0f },
        { 5.0f, 15.0f },
        { 2.0f, 5.0f },
    };

    int idx = std::clamp(biomeId, 0, 14);
    float targetSpeed = biomeWinds[idx].min +
        (biomeWinds[idx].max - biomeWinds[idx].min) * 0.5f;

    targetSpeed *= (1.0f + weatherIntensity * 0.5f);
    if (overrideSpeed >= 0.0f) targetSpeed = overrideSpeed;

    speed += (targetSpeed - speed) * std::min(1.0f, dt * (overrideSpeed >= 0.0f ? 1.5f : 0.3f));

    gustTimer += dt;
    if (gustTimer > 4.0f + (rand() % 100) / 100.0f * 3.0f) {
        gust = (overrideSpeed >= 0.0f) ? 0.0f : (float)(rand() % 100) / 100.0f;
        gustTimer = 0.0f;
    }

    gust *= std::max(0.0f, 1.0f - dt * 0.8f);

    if (gustTimer > 3.0f) {
        targetDir += ((rand() % 200) - 100) / 100.0f * 0.4f;
    }
    currentDir += (targetDir - currentDir) * std::min(1.0f, dt * 0.15f);

    direction.x = cosf(currentDir);
    direction.y = 0.1f;
    direction.z = sinf(currentDir);
    direction = Vector3Normalize(direction);
}

Vector3 WindSystem::GetWindVector() const {
    float totalSpeed = speed * (1.0f + gust);
    return { direction.x * totalSpeed, direction.y * totalSpeed, direction.z * totalSpeed };
}

std::string WindSystem::GetWindInfo(float playerYaw) const {

    float windYaw = atan2f(direction.z, direction.x);
    float relAngle = windYaw - playerYaw;

    while (relAngle > PI) relAngle -= 2 * PI;
    while (relAngle < -PI) relAngle += 2 * PI;

    const char* dirStr = "";
    if (relAngle > -0.5f && relAngle < 0.5f) dirStr = "[^]";
    else if (relAngle >= 0.5f && relAngle < 1.3f) dirStr = "[>]";
    else if (relAngle >= 1.3f && relAngle < 1.9f) dirStr = "[v]";
    else if (relAngle >= 1.9f || relAngle <= -1.9f) dirStr = "[v]";
    else if (relAngle <= -0.5f && relAngle > -1.3f) dirStr = "[<]";
    else dirStr = "[v]";

    std::ostringstream oss;
    oss << "Wind: " << std::fixed << std::setprecision(1)
        << (speed * (1.0f + gust)) << " m/s " << dirStr;
    if (gust > 0.3f) oss << " ⚡";
    return oss.str();
}
