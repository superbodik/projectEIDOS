#pragma once
#include <raylib.h>

class EidosEngine;

class DebugOverlay {
public:
    void Init(EidosEngine* engine);
    void Toggle() { isVisible = !isVisible; }
    void Render();

    bool IsVisible() const { return isVisible; }

private:
    EidosEngine* engine = nullptr;
    bool isVisible = false;
};