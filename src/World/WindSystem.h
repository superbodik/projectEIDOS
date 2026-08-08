#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>

class WindSystem {
public:
    WindSystem();

    void Update(float dt, int biomeId, float weatherIntensity);

    Vector3 GetDirection() const { return direction; }

    float GetSpeed() const { return speed; }

    float GetGust() const { return gust; }

    Vector3 GetWindVector() const;

    std::string GetWindInfo(float playerYaw) const;

    void SetOverride(float force) { overrideSpeed = force; }
    void ClearOverride() { overrideSpeed = -1.0f; }
    bool HasOverride() const { return overrideSpeed >= 0.0f; }

private:
    Vector3 direction;
    float speed;
    float gust;
    float gustTimer;
    float targetDir;
    float currentDir;
    float overrideSpeed = -1.0f;
};
