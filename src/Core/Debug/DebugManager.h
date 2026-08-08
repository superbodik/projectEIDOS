#pragma once
#include "Logger.h"
#include "GameConsole.h"
#include "DebugOverlay.h"
#include <sol/sol.hpp>

class EidosEngine;

class DebugManager {
public:
    DebugManager();
    ~DebugManager();

    void Initialize(EidosEngine* engine, sol::state* lua);
    void Update();
    void Render2D();

    void Log(const std::string& msg) { logger.Log(msg); }

    void ToggleConsole() { console.Toggle(); }
    bool IsConsoleOpen() const { return console.IsOpen(); }

    void ToggleOverlay() { overlay.Toggle(); }

    bool showChunkBorders = false;

private:
    Logger logger;
    GameConsole console;
    DebugOverlay overlay;
    EidosEngine* engine = nullptr;
};
