#pragma once
#include <string>

enum class Difficulty { Casual = 0, Survival = 1, Hardcore = 2, Custom = 3 };

struct WorldRules {
    Difficulty difficulty = Difficulty::Survival;

    float dayLength = 480.0f;
    float hungerRate = 1.0f;
    float thirstRate = 1.0f;
    float nutrientDecay = 1.0f;
    float damageScale = 1.0f;

    float regenFloor = 0.35f;
    float regenSatietyGate = 0.55f;
    float regenThirstGate = 0.45f;

    float comfortLow = 12.0f;
    float comfortHigh = 26.0f;
    float coldThreshold = 33.0f;
    float heatThreshold = 39.5f;

    int   deathPenalty = 1;
    bool  oneLife = false;

    float oreDensity = 1.0f;
    float treeDensity = 1.0f;
    float caveDensity = 1.0f;
    float latitudeScale = 7000.0f;

    bool  questsEnabled = true;

    static WorldRules Preset(Difficulty d);
    static const char* Name(Difficulty d);
    static const char* Blurb(Difficulty d);

    std::string Serialize() const;
    bool Deserialize(const std::string& line);
};
