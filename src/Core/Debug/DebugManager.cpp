#include "DebugManager.h"
#include "../EidosEngine.h" 

DebugManager::DebugManager() {}
DebugManager::~DebugManager() {}

void DebugManager::Initialize(EidosEngine* eng, sol::state* lua) {
    this->engine = eng;

    logger.Init();
    console.Init(&logger, lua, eng);
    overlay.Init(eng);

    logger.Log("Debug System Initialized.");
}

void DebugManager::Update() {
    console.Update();

    if (IsKeyDown(KEY_F3) && IsKeyPressed(KEY_G)) {
        showChunkBorders = !showChunkBorders;
    }
    else if (IsKeyPressed(KEY_F3)) {
        overlay.Toggle();
    }
}

void DebugManager::Render2D() {
    overlay.Render(); 
    console.Render();
}