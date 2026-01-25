#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>

class SkySystem {
public:
    SkySystem();

    void Update(float dt, Vector3 playerPos);

    void Draw();

    Color GetFogColor() const;

    Color GetSkyColor() const;

    float GetTime() const { return timeOfDay; }
    void SetTime(float t) { timeOfDay = t; }

private:
    float timeOfDay = 0.0f; 
    float timeSpeed = 0.02f; 

    Vector3 sunPosition;
    Vector3 moonPosition;
    Vector3 centerPos; 

    Color currentSkyColor;
    Color currentFogColor;

    Color dayTop = { 100, 190, 255, 255 };   
    Color dayBottom = { 200, 230, 255, 255 }; 

    Color sunsetTop = { 60, 50, 100, 255 };   
    Color sunsetBottom = { 255, 130, 50, 255 };

    Color nightTop = { 5, 5, 20, 255 };      
    Color nightBottom = { 10, 10, 30, 255 }; 

    void CalculateCelestialPositions();
    void CalculateColors();
    Color LerpColor(Color a, Color b, float t);
};