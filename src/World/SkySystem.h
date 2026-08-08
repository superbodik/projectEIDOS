#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>

class SkySystem {
public:
    SkySystem();

    void Update(float dt, Vector3 playerPos);

    void Draw(const Camera3D& cam);
    void DrawClouds(Vector3 windDir, float windSpeed);
    void DrawMotes(Vector3 playerPos, Vector3 windDir, float windSpeed);

    Color GetFogColor() const;

    Color GetSkyColor() const;

    float GetTime() const { return timeOfDay; }
    void SetTime(float t) { timeOfDay = t; }
    Vector3 GetSunPosition() const { return sunPosition; }

private:
    float timeOfDay = 0.0f;
    float timeSpeed = 0.02f;

    Vector3 sunPosition;
    Vector3 moonPosition;
    Vector3 centerPos;

    Color currentSkyColor;
    Color currentFogColor;

    struct Cloud { float x, z, y, size; };
    std::vector<Cloud> clouds;
    void InitClouds();

    struct Mote { Vector3 pos; float phase; };
    std::vector<Mote> motes;

    Texture2D sunRayTex = { 0 };
    void EnsureSunTexture();

    void CalculateCelestialPositions();
    void CalculateColors();
    Color LerpColor(Color a, Color b, float t);
};
