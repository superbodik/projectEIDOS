#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class EidosEngine;

struct ShotStep {
    std::string name;
    int  tab = -1;
    bool inventoryOpen = false;
    bool questOpen = false;
    int  viewMode = 0;
    int  holdBlock = 0;
    int  equipHead = 0;
    int  equipChest = 0;
    int  cosmeticHat = 0;
    int  cosmeticCape = 0;
    bool creative = false;

    bool  biomeShot = false;
    int   biomeId = -1;
    float skyTime = 0.42f;
};

class AutoShot {
public:
    static bool Enabled();

    void Init();
    bool Active() const { return active; }

    void Update(EidosEngine& eng);
    void EndFrame(EidosEngine& eng);

private:
    bool active = false;
    bool wantCapture = false;
    std::vector<ShotStep> steps;
    int   stepIndex = 0;
    int   waited = 0;
    float settleClock = 0.0f;
    bool  placed = false;
    bool  biomeMode = false;
    int   anchorX = 0;
    int   anchorZ = 0;

    void InitBiomeTour();
    std::string outputDir = "ui_shots";

    void Apply(EidosEngine& eng, const ShotStep& s);
    void Capture(const std::string& name);
};
