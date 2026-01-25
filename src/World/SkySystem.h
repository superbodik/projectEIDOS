#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>

class SkySystem {
public:
    SkySystem();

    // dt - дельта времени, playerPos - чтобы небо "ездило" за игроком
    void Update(float dt, Vector3 playerPos);

    // Рисуем небо, солнце и луну
    void Draw();

    // Получить цвет тумана для шейдера (чтобы горизонт сливался с небом)
    Color GetFogColor() const;

    // Получить текущий цвет неба (для ClearBackground)
    Color GetSkyColor() const;

    // Управление временем (0.0 - 1.0)
    // 0.0 = Полдень, 0.5 = Полночь
    float GetTime() const { return timeOfDay; }
    void SetTime(float t) { timeOfDay = t; }

private:
    float timeOfDay = 0.0f; // 0.0 ... 1.0
    float timeSpeed = 0.02f; // Скорость течения времени

    Vector3 sunPosition;
    Vector3 moonPosition;
    Vector3 centerPos; // Позиция игрока

    // Цвета для интерполяции
    Color currentSkyColor;
    Color currentFogColor;

    Color dayTop = { 100, 190, 255, 255 };    // Голубое небо
    Color dayBottom = { 200, 230, 255, 255 }; // Светлый горизонт

    Color sunsetTop = { 60, 50, 100, 255 };   // Фиолетовый верх
    Color sunsetBottom = { 255, 130, 50, 255 }; // Оранжевый горизонт

    Color nightTop = { 5, 5, 20, 255 };       // Почти черный
    Color nightBottom = { 10, 10, 30, 255 };  // Темно-синий

    void CalculateCelestialPositions();
    void CalculateColors();
    Color LerpColor(Color a, Color b, float t);
};