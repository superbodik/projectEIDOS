#include "../EidosEngine.h"
#include <unordered_map>
#include "../CommandManager.h"
#include "../../Cinematic/Trailer.h"
#include "../../Cinematic/AutoShot.h"
#include "../../Inventory/BlockInfo.h"
#include "../../Inventory/MiningRules.h"
#include "../../Progression/QuestSystem.h"
#include "../../World/Chunk.h"
#include "rlgl.h"
#include <raymath.h>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <deque>
#include <chrono>

namespace fs = std::filesystem;

void EidosEngine::LoadConfig() {
    std::string path = "config/settings.ini";
    if (!fs::exists("config")) fs::create_directories("config");
    if (fs::exists(path)) {
        std::ifstream file(path); std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line); std::string key; ss >> key;
            if (key == "RenderDistance") { int val; if (ss >> val) SetRenderDistance(val); }
            else if (key == "FOV") { float val; if (ss >> val) SetFOV(val); }
            else if (key == "MaxFPS") { int val; if (ss >> val) SetMaxFPS(val); }
            else if (key == "ShowFPS") { int val; if (ss >> val) showSimpleFPS = (val != 0); }
            else if (key == "ShowWind") { int val; if (ss >> val) showWind = (val != 0); }
            else if (key == "Fullscreen") { int val; if (ss >> val && val == 1) ToggleFullscreen(); }
            else if (key == "WindowW") { int val; if (ss >> val && val >= 854) windowedW = val; }
            else if (key == "WindowH") { int val; if (ss >> val && val >= 480) windowedH = val; }
            else if (key == "GuiScale") { int val; if (ss >> val && val >= 0 && val <= 3) OverlayUI::guiScaleSetting = val; }
        }
    }
    else { SetRenderDistance(8); SetMaxFPS(165); SetFOV(70.0f); }
}

void EidosEngine::SaveConfig() {
    if (!fs::exists("config")) fs::create_directories("config");
    std::ofstream file("config/settings.ini");
    if (file.is_open()) {
        file << "RenderDistance " << renderDistance << "\n";
        file << "FOV " << targetFOV << "\n";
        file << "MaxFPS " << targetFPS << "\n";
        file << "ShowFPS " << (showSimpleFPS ? 1 : 0) << "\n";
        file << "ShowWind " << (showWind ? 1 : 0) << "\n";
        bool borderless = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
        file << "Fullscreen " << (borderless ? 1 : 0) << "\n";
        file << "WindowW " << (borderless ? windowedW : GetScreenWidth()) << "\n";
        file << "WindowH " << (borderless ? windowedH : GetScreenHeight()) << "\n";
        file << "GuiScale " << OverlayUI::guiScaleSetting << "\n";
        file.close();
    }
}

void EidosEngine::CaptureScreenshot() {
    if (!fs::exists("screenshots")) fs::create_directories("screenshots");
    auto t = std::time(nullptr); auto tm = *std::localtime(&t);
    std::ostringstream oss; oss << "screenshots/" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".png";
    TakeScreenshot(oss.str().c_str()); debugSystem.Log("Screenshot saved.");
}

void EidosEngine::SetRenderDistance(int dist) {
    if (dist < 2) dist = 2; if (dist > 32) dist = 32;
    renderDistance = dist;
}

int EidosEngine::GetRenderDistance() const { return renderDistance; }
void EidosEngine::SetMaxFPS(int fps) { targetFPS = fps; SetTargetFPS(fps <= 0 ? 0 : fps); }
int EidosEngine::GetMaxFPS() const { return targetFPS; }
void EidosEngine::SetFOV(float fov) { targetFOV = std::clamp(fov, 30.0f, 110.0f); player.camera.fovy = targetFOV; }
float EidosEngine::GetFOV() const { return targetFOV; }
float EidosEngine::GetUIScale() const { return (float)GetScreenHeight() / 1080.0f; }
bool EidosEngine::ShouldClose() const { return WindowShouldClose() || !appRunning; }

void EidosEngine::ToggleFullscreen() {
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        ToggleBorderlessWindowed();
        if (windowedW > 0 && windowedH > 0) SetWindowSize(windowedW, windowedH);
    }
    else {
        if (!IsWindowMaximized()) {
            windowedW = GetScreenWidth();
            windowedH = GetScreenHeight();
        }
        ToggleBorderlessWindowed();
    }
}

