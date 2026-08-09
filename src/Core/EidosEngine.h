#pragma once
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <string>
#include <memory>
#include <sol/sol.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

#include "Debug/DebugManager.h"
#include "../UI/MenuSystem.h"
#include "../UI/OverlayUI.h"
#include "../Entities/Player.h"
#include "../World/Chunk.h"
#include "../World/WorldGenerator.h"
#include "../World/SkySystem.h"
#include "../World/WindSystem.h"
#include "WorldRules.h"

class CommandManager;
class QuestSystem;
class Trailer;
class AutoShot;

struct RayHitInfo {
    bool hit;
    int x, y, z;
    int px, py, pz;
};

enum class GameState {
    MainMenu,
    Settings,
    WorldSelect,
    CreateWorld,
    Loading,
    Playing,
    Paused
};

class EidosEngine {
public:
    EidosEngine(int width, int height, std::string title);
    ~EidosEngine();

    bool ShouldClose() const;
    void CloseApp();
    void ToggleFullscreen();

    void Update();
    void Render();

    Player& GetPlayer();
    void ToggleDebug();
    void Locate(std::string type, std::string value);

    void SetRenderDistance(int dist);
    int GetRenderDistance() const;

    void SetMaxFPS(int fps);
    int GetMaxFPS() const;

    void SetFOV(float fov);
    float GetFOV() const;

    float GetUIScale() const;

    void LoadConfig();
    void SaveConfig();
    void CaptureScreenshot();

    void SetPlayerPosition(Vector3 p);
    void SetSkyTime(float t);
    void ApplyGenSettings();
    void SetQuestTreeOpen(bool open);
    bool cinematicMode = false;

    WorldRules rules;
    bool worldDead = false;
    int  randomTickSpeed = 3;

    void SetBlockGlobal(int x, int y, int z, int type);
    void SetBlockGlobalFast(int x, int y, int z, int type);

    BlockType GetBlockAt(int x, int y, int z);
    Chunk* GetChunkAt(int x, int z);

    std::string GetBlockName(int type) const;
    RayHitInfo CastRay(float maxDistance);

    WorldGenerator worldGen;
    std::vector<std::shared_ptr<Chunk>> chunks;
    std::recursive_mutex chunkListMutex;

    GameState currentState = GameState::MainMenu;
    std::string currentWorldName = "World1";

    bool showSimpleFPS = true;
    bool showWind = false;
    bool hudVisible = true;

    void SaveWorld(bool autoSave = false);
    void LoadWorld(std::string worldName);
    void UnloadWorld();

    size_t GetQueueSize();

    std::string GetBiomeName(int x, int y, int z);

    DebugManager debugSystem;
    SkySystem skySystem;
    WindSystem windSystem;

    void LoadPanorama();
    void UnloadPanorama();
    void DrawPanorama();
private:
    int screenWidth;
    int screenHeight;
    int windowedW = 1280;
    int windowedH = 720;
    bool showChunkBorders = false;

    int renderDistance = 8;
    float targetFOV = 70.0f;
    int targetFPS = 165;

    Shader fogShader;
    int fogViewPosLoc;
    int fogColorLoc;
    int fogWindVecLoc;
    int fogWindTimeLoc;
    int fogStartLoc;
    int fogEndLoc;
    int fogSunDirLoc;

    Shader waterShader;
    int waterViewPosLoc;
    int waterFogColorLoc;
    int waterFogStartLoc;
    int waterFogEndLoc;
    int waterTimeLoc;
    int waterSunDirLoc;
    int waterSkyTintLoc;
    int waterUnderwaterLoc;
    int waterWindLoc;
    bool cameraUnderwater = false;

    void UpdateWaterUniforms();
    bool IsCameraUnderwater();

    float climateTimer = 0.0f;
    void UpdateClimate(float dt);

    float survivalTimer = 0.0f;
    void UpdateSurvival(float dt);
    bool TryEatHeldItem();
    bool TryShapeClay();
    bool TryMakePlanks();
    bool TryMakeSticks();
    bool TryMakeRope();
    bool TryKnap();
    bool TryAssembleTool();
    void GrantForage(int brokenId, int x, int y, int z);
    void DrawFirstPersonHands(int heldBlockId);
    void DrawPlayerNametag();

    float worldTickTimer = 0.0f;
    unsigned int worldTickSeed = 20260801u;
    void UpdateWorldTicks(float dt);
    void TickBlock(int x, int y, int z);
    void TryGrowOak(int x, int y, int z);
    void TryDropAcorn(int x, int y, int z);
    bool HasLogNearby(int x, int y, int z);

    Shader shadowShader;
    RenderTexture2D shadowMap;
    int shadowMVPLoc;
    int shadowMapTexLoc;
    int shadowStrengthLoc = -1;
    int exposureLoc = -1;
    int saturationLoc = -1;
    float shadowStrength = 0.0f;
    float toneExposure = 1.0f;
    float toneSaturation = 1.06f;
    void RenderShadowPass();

    sol::state lua;
    std::unique_ptr<MenuSystem> menuSystem;
    Player player;
    std::unique_ptr<CommandManager> cmdManager;
    std::unique_ptr<QuestSystem> questSystem;
    std::unique_ptr<Trailer> trailer;
    std::unique_ptr<AutoShot> autoShot;

    std::atomic<bool> appRunning{ true };
    std::vector<std::thread> threadPool;

    std::queue<std::shared_ptr<Chunk>> terrainQueue;
    std::queue<std::shared_ptr<Chunk>> meshQueue;
    std::mutex terrainQueueMutex;
    std::mutex meshQueueMutex;
    float autoSaveTimer = 0.0f;

    void TerrainWorker();
    void MeshWorker();

    std::string GetDirectionString(float rotationY) const;

    void UpdateChunks();
    bool IsChunkInFrustum(Chunk* chunk) const;
    bool IsAreaLoaded(int radius);

    Texture2D panoramaTextures[6];
    bool hasPanorama = false;
};
