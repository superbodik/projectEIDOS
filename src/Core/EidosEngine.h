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
#include "MenuSystem.h" 
#include "../Entities/Player.h"
#include "../World/Chunk.h"
#include "../World/WorldGenerator.h"

class CommandManager;

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
    void CloseApp() { appRunning = false; }
    void ToggleFullscreen();

    void Update();
    void Render();

    Player& GetPlayer() { return player; }
    void ToggleDebug() { debugSystem.ToggleOverlay(); }

    // --- НАСТРОЙКИ ---
    void SetRenderDistance(int dist);
    int GetRenderDistance() const { return renderDistance; }

    void SetMaxFPS(int fps);
    int GetMaxFPS() const { return targetFPS; } // Геттер для сохранения

    void SetFOV(float fov);
    float GetFOV() const { return targetFOV; }

    float GetUIScale() const;

    // --- НОВЫЕ МЕТОДЫ ---
    void LoadConfig();       // Загрузка настроек из файла
    void SaveConfig();       // Сохранение настроек в файл
    void CaptureScreenshot(); // Скриншот (F2)
    // --------------------

    void SetBlockGlobal(int x, int y, int z, int type);
    void SetBlockGlobalFast(int x, int y, int z, int type);

    BlockType GetBlockAt(int x, int y, int z);
    Chunk* GetChunkAt(int x, int z);

    std::string GetBlockName(int type) const;
    RayHitInfo CastRay(float maxDistance);

    WorldGenerator worldGen;
    std::vector<std::shared_ptr<Chunk>> chunks;

    GameState currentState = GameState::MainMenu;
    std::string currentWorldName = "World1";

    bool showSimpleFPS = true;

    void SaveWorld();
    void LoadWorld(std::string worldName);
    void UnloadWorld();

    size_t GetQueueSize() {
        std::lock_guard<std::mutex> lk(queueMutex);
        return generationQueue.size();
    }

    std::string GetBiomeName(int x, int y, int z);

    DebugManager debugSystem;

private:
    int screenWidth;
    int screenHeight;
    bool showChunkBorders = false;

    int renderDistance = 8;
    float targetFOV = 70.0f;
    int targetFPS = 165;

    Shader fogShader;
    int fogViewPosLoc;

    sol::state lua;
    std::unique_ptr<MenuSystem> menuSystem;
    Player player;
    std::unique_ptr<CommandManager> cmdManager;

    std::atomic<bool> appRunning{ true };
    std::vector<std::thread> threadPool;

    std::mutex queueMutex;
    std::queue<std::shared_ptr<Chunk>> generationQueue;
    std::recursive_mutex chunkListMutex;

    void GeneratorThreadWorker();
    void DrawLoadingScreen();

    std::string GetDirectionString(float rotationY) const;

    void UpdateChunks();
    bool IsChunkInFrustum(Chunk* chunk) const;
    bool IsAreaLoaded(int radius);
};