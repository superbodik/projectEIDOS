#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class EidosEngine;

struct TrailerShot {
    std::string caption;
    std::string subcaption;
    Vector3 fromOffset = { 0, 0, 0 };
    Vector3 toOffset = { 0, 0, 0 };
    Vector3 lookOffset = { 0, 0, 0 };
    float   duration = 6.0f;
    float   timeOfDay = 10.0f;
    float   fov = 65.0f;
    int     biome = -1;
    int     anchorX = 0;
    int     anchorZ = 0;
    float   anchorY = 0.0f;
    bool    resolved = false;
    bool    orbit = false;
    float   orbitDegrees = 26.0f;

    bool    showPlayer = false;
    bool    showInventory = false;
    int     inventoryTab = 0;
    bool    showQuests = false;
    bool    creative = false;
    int     heldBlock = 0;
};

class Trailer {
public:
    static bool Enabled();

    void Init(EidosEngine& eng);
    bool Active() const { return active; }

    void Update(EidosEngine& eng, Camera3D& cam, float& skyTime);
    void DrawOverlay(int sw, int sh);
    void EndFrame();

    int  FrameCount() const { return frameIndex; }
    bool Finished() const { return finished; }
    const std::string& OutputDir() const { return outputDir; }

private:
    bool active = false;
    bool finished = false;

    std::vector<TrailerShot> shots;
    int   shotIndex = 0;
    float shotClock = 0.0f;
    int   settleFrames = 0;
    float settleClock = 0.0f;
    int   frameIndex = 0;

    float fps = 30.0f;
    float fadeIn = 1.0f;
    bool  wantCapture = false;
    std::string outputDir = "trailer_frames";

    int   shotFrames = 0;
    bool  allResolved = false;
    int   lastAnchorX = 0;
    int   lastAnchorZ = 0;

    void ApplyShotState(EidosEngine& eng, const TrailerShot& shot);
    void ResolveShot(EidosEngine& eng, TrailerShot& shot);
    void ApplyCamera(const TrailerShot& shot, float t, Camera3D& cam);
    void ClearCamera(EidosEngine& eng, Camera3D& cam);
    void Capture();
};
