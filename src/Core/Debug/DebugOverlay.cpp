#include "DebugOverlay.h"
#include "../EidosEngine.h"
#include <cmath>

void DebugOverlay::Init(EidosEngine* eng) {
    this->engine = eng;
}

void DebugOverlay::Render() {
    if (!isVisible || !engine) return;

    auto& player = engine->GetPlayer();
    Vector3 pos = player.position;

    std::string biome = engine->GetBiomeName((int)pos.x, (int)pos.y, (int)pos.z);

    Vector3 fwd = Vector3Subtract(player.camera.target, player.camera.position);
    float angle = atan2(fwd.x, fwd.z) * RAD2DEG;
    if (angle < 0) angle += 360;

    const char* dirStr = "N/A";
    if (angle >= 315 || angle < 45) dirStr = "North (-Z)";
    else if (angle >= 45 && angle < 135) dirStr = "East (+X)";
    else if (angle >= 135 && angle < 225) dirStr = "South (+Z)";
    else dirStr = "West (-X)";

    int y = 10;
    int dy = 20;
    int fontSize = 20;
    Color col = WHITE;

    DrawText("EIDOS Engine 0.0.0_1(TETS_ALPHA)", 10, y, fontSize, GREEN); y += dy;
    DrawText(TextFormat("FPS: %d (Target: %d)", GetFPS(), 165), 10, y, fontSize, col); y += dy;
    y += 10;

    DrawText(TextFormat("XYZ: %.2f / %.2f / %.2f", pos.x, pos.y, pos.z), 10, y, fontSize, col); y += dy;
    DrawText(TextFormat("Block: %d %d %d", (int)floor(pos.x), (int)floor(pos.y), (int)floor(pos.z)), 10, y, fontSize, col); y += dy;
    DrawText(TextFormat("Chunk: %d %d", (int)floor(pos.x) >> 4, (int)floor(pos.z) >> 4), 10, y, fontSize, col); y += dy;
    y += 10;

    DrawText(TextFormat("Facing: %s (%.1f)", dirStr, angle), 10, y, fontSize, col); y += dy;
    DrawText(TextFormat("Biome: %s", biome.c_str()), 10, y, fontSize, col); y += dy;
    y += 10;

    size_t qSize = engine->GetQueueSize();
    DrawText(TextFormat("Gen Queue: %llu", qSize), 10, y, fontSize, (qSize > 10) ? RED : GREEN); y += dy;

    RayHitInfo hit = engine->CastRay(5.0f);
    if (hit.hit) {
        std::string blockName = engine->GetBlockName((int)engine->GetBlockAt(hit.x, hit.y, hit.z));
        DrawText(TextFormat("Looking at: %s (%d %d %d)", blockName.c_str(), hit.x, hit.y, hit.z), 10, y, fontSize, YELLOW);
    }
    else {
        DrawText("Looking at: None", 10, y, fontSize, GRAY);
    }
}