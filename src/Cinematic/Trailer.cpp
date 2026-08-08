#include "Trailer.h"
#include "../Core/EidosEngine.h"
#include "../World/WorldGenerator.h"
#include <raymath.h>
#include <rlgl.h>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <cmath>

namespace {

    float EaseInOut(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float EaseOut(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

}

bool Trailer::Enabled() {
    const char* v = std::getenv("EIDOS_TRAILER");
    return v && v[0] && v[0] != '0';
}

void Trailer::Init(EidosEngine& eng) {
    active = true;
    finished = false;

    eng.cinematicMode = true;
    int dist = 14;
    if (const char* d = std::getenv("EIDOS_TRAILER_DIST")) dist = atoi(d);
    eng.SetRenderDistance(std::clamp(dist, 6, 24));

    if (const char* d = std::getenv("EIDOS_TRAILER_DIR")) outputDir = d;
    if (const char* f = std::getenv("EIDOS_TRAILER_FPS")) {
        float v = (float)atof(f);
        if (v > 1.0f && v <= 120.0f) fps = v;
    }

    int tw = 1920, th = 1080;
    if (const char* w = std::getenv("EIDOS_TRAILER_W")) tw = atoi(w);
    if (const char* h = std::getenv("EIDOS_TRAILER_H")) th = atoi(h);
    if (tw >= 640 && th >= 360) {
        SetWindowSize(tw, th);
        SetWindowPosition(20, 20);
    }

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);

    auto shot = [&](const char* cap, const char* sub, int biome,
        Vector3 from, Vector3 to, Vector3 look,
        float dur, float tod, float fov, bool orbit = false) -> TrailerShot& {
            TrailerShot s;
            s.caption = cap ? cap : "";
            s.subcaption = sub ? sub : "";
            s.biome = biome;
            s.fromOffset = from;
            s.toOffset = to;
            s.lookOffset = look;
            s.duration = dur;
            s.timeOfDay = tod;
            s.fov = fov;
            s.orbit = orbit;
            shots.push_back(s);
            return shots.back();
        };

    shot("CRC EIDOS", "From primal matter to absolute control",
        (int)BiomeType::Mountains,
        { -34, 16, -34 }, { -19, 11, -19 }, { 0, 3, 0 }, 7.0f, 7.5f, 62.0f, true);

    shot("Rivers carve their own valleys", "pools, sills and waterfalls down to the sea",
        (int)BiomeType::River,
        { -19, 7, -16 }, { 15, 4, 13 }, { 0, 1, 0 }, 7.0f, 9.5f, 66.0f);

    {
        TrailerShot& s = shot("You are here", "third person, armour and cosmetics",
            (int)BiomeType::TemperateDeciduousForest,
            { -7, 3.2f, -7 }, { -4, 2.4f, -4 }, { 0, 1.7f, 0 }, 6.0f, 11.0f, 62.0f);
        s.showPlayer = true;
        s.heldBlock = (int)BlockType::Torch;
    }

    {
        TrailerShot& s = shot("Real geology underfoot",
            "five provinces, ore bound to its host rock",
            (int)BiomeType::Mountains,
            { -11, 4.5f, -11 }, { -6, 3, -6 }, { 0, 2, 0 }, 7.0f, 12.0f, 64.0f);
        s.heldBlock = (int)BlockType::GranitePebble;
    }

    {
        TrailerShot& s = shot("Survival with teeth",
            "hunger, thirst, body temperature - and a diet that caps your healing",
            (int)BiomeType::TemperateDeciduousForest,
            { -9, 3.4f, -9 }, { -8, 3.2f, -8 }, { 0, 1.7f, 0 }, 7.5f, 12.5f, 60.0f);
        s.showInventory = true;
        s.inventoryTab = 2;
        s.heldBlock = (int)BlockType::Berries;
    }

    {
        TrailerShot& s = shot("Every block, every rock",
            "104 blocks and counting",
            (int)BiomeType::TemperateRainforest,
            { -9, 3.4f, -9 }, { -8, 3.2f, -8 }, { 0, 1.7f, 0 }, 6.5f, 13.0f, 60.0f);
        s.showInventory = true;
        s.inventoryTab = 0;
        s.creative = true;
    }

    {
        TrailerShot& s = shot("Ten eras of automation",
            "Era I is playable now - 23 quests from two stones in the mud",
            (int)BiomeType::TemperateDeciduousForest,
            { -9, 3.4f, -9 }, { -8, 3.2f, -8 }, { 0, 1.7f, 0 }, 8.0f, 13.5f, 60.0f);
        s.showQuests = true;
    }

    shot("Climate follows latitude", "walk north until the world freezes",
        (int)BiomeType::SnowyTundra,
        { -22, 9, -19 }, { 13, 5, 14 }, { 0, 2, 0 }, 6.5f, 10.5f, 66.0f);

    shot("The world keeps living without you", "grass spreads, acorns become trees",
        (int)BiomeType::TropicalRainforest,
        { -15, 6, -13 }, { 8, 4, 7 }, { 0, 3, 0 }, 6.5f, 16.5f, 66.0f);

    shot("CRC EIDOS", "closed beta - t.me/CRCEIODOS",
        (int)BiomeType::Mountains,
        { 28, 15, 28 }, { 17, 11, 17 }, { 0, 4, 0 }, 7.0f, 18.4f, 58.0f, true);

    TraceLog(LOG_INFO, "[trailer] %d shots -> %s at %.0f fps",
        (int)shots.size(), outputDir.c_str(), fps);
}

void Trailer::ResolveShot(EidosEngine& eng, TrailerShot& shot) {
    if (shot.resolved) return;

    int ax = lastAnchorX, az = lastAnchorZ;
    if (shot.biome >= 0) {
        BiomeSearchResult r = eng.worldGen.FindBiome(
            (shot.biome * 977) % 3000 - 1500, (shot.biome * 613) % 3000 - 1500,
            (BiomeType)shot.biome, 7000, 160);
        if (r.found) { ax = r.x; az = r.z; }
    }
    lastAnchorX = ax;
    lastAnchorZ = az;

    int peak = eng.worldGen.GetHeight(ax, az);
    for (int dx = -24; dx <= 24; dx += 12) {
        for (int dz = -24; dz <= 24; dz += 12) {
            int h = eng.worldGen.GetHeight(ax + dx, az + dz);
            if (h > peak) peak = h;
        }
    }

    shot.anchorX = ax;
    shot.anchorZ = az;
    shot.anchorY = (float)std::max(peak, 62);
    shot.resolved = true;

    FILE* f = fopen((outputDir + "/report.txt").c_str(), "a");
    if (f) {
        fprintf(f, "resolve '%s' at %d,%d ground=%.0f\n",
            shot.caption.c_str(), ax, az, shot.anchorY);
        fclose(f);
    }
}

void Trailer::ApplyCamera(const TrailerShot& shot, float t, Camera3D& cam) {
    float e = EaseInOut(t);

    Vector3 base = { (float)shot.anchorX, shot.anchorY, (float)shot.anchorZ };
    Vector3 from = shot.fromOffset;
    Vector3 to = shot.toOffset;

    if (shot.orbit) {
        float ang = (shot.orbitDegrees * DEG2RAD) * e;
        float ca = cosf(ang), sa = sinf(ang);
        Vector3 r = {
            from.x * ca - from.z * sa,
            std::lerp(from.y, to.y, e),
            from.x * sa + from.z * ca
        };
        float scale = std::lerp(1.0f, Vector3Length({ to.x, 0, to.z }) /
            std::max(0.001f, Vector3Length({ from.x, 0, from.z })), e);
        from = { r.x * scale, r.y, r.z * scale };
        cam.position = Vector3Add(base, from);
    }
    else {
        cam.position = Vector3Add(base, Vector3Lerp(from, to, e));
    }

    cam.target = Vector3Add(base, shot.lookOffset);
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = shot.fov;
    cam.projection = CAMERA_PERSPECTIVE;
}

void Trailer::ClearCamera(EidosEngine& eng, Camera3D& cam) {
    int cx = (int)floorf(cam.position.x);
    int cz = (int)floorf(cam.position.z);

    int ground = eng.worldGen.GetHeight(cx, cz);
    float floorY = (float)std::max(ground, 62) + 2.5f;

    if (cam.position.y < floorY) {
        float lift = floorY - cam.position.y;
        cam.position.y = floorY;
        cam.target.y += lift * 0.6f;
    }

    for (int i = 0; i < 24; i++) {
        BlockType b = (BlockType)eng.GetBlockAt(cx, (int)floorf(cam.position.y), cz);
        if (b == BlockType::Air) break;
        cam.position.y += 1.0f;
        cam.target.y += 0.4f;
    }
}

void Trailer::ApplyShotState(EidosEngine& eng, const TrailerShot& shot) {
    Player& p = eng.GetPlayer();

    p.name = "superbodik";
    p.currentMode = shot.creative ? GameMode::Creative : GameMode::Survival;
    p.viewMode = shot.showPlayer ? Player::VIEW_BACK : Player::VIEW_FIRST;

    p.health = 0.82f;
    p.satiety = 0.68f;
    p.hydration = 0.74f;
    p.bodyTempC = 36.6f;
    p.nutrients[0] = 0.54f;
    p.nutrients[1] = 0.39f;
    p.nutrients[2] = 0.29f;
    p.nutrients[3] = 0.44f;

    p.appearance.equipment[(int)EquipSlot::Head] = (int)BlockType::Hematite;
    p.appearance.equipment[(int)EquipSlot::Chest] = (int)BlockType::NativeCopper;
    p.appearance.cosmetics[(int)CosmeticSlot::Hat] = (int)BlockType::NativeGold;
    p.appearance.cosmetics[(int)CosmeticSlot::Cape] = (int)BlockType::Berries;

    p.inventory.isOpen = shot.showInventory;
    if (shot.showInventory) p.inventory.activeTab = (Inventory::Tab)shot.inventoryTab;

    p.inventory.slots[0] = { shot.heldBlock ? shot.heldBlock : (int)BlockType::Torch, 9 };
    p.inventory.slots[1] = { (int)BlockType::Berries, 19 };
    p.inventory.slots[2] = { (int)BlockType::Acorn, 6 };
    p.inventory.slots[3] = { (int)BlockType::Grubs, 3 };
    p.inventory.slots[4] = { (int)BlockType::FlintPebble, 8 };
    p.inventory.slots[5] = { (int)BlockType::GranitePebble, 4 };
    p.inventory.slots[6] = { (int)BlockType::OakLog, 24 };
    p.inventory.slots[9] = { (int)BlockType::Limestone, 41 };
    p.inventory.slots[10] = { (int)BlockType::Marble, 17 };
    p.inventory.slots[11] = { (int)BlockType::BituminousCoal, 12 };
    p.inventory.slots[12] = { (int)BlockType::BirdEgg, 2 };
    p.inventory.currentSlotIndex = 0;

    eng.SetQuestTreeOpen(shot.showQuests);
    eng.hudVisible = (shot.showInventory || shot.showQuests || shot.showPlayer);
}

void Trailer::Update(EidosEngine& eng, Camera3D& cam, float& skyTime) {
    if (!active || finished) return;

    if (shotIndex >= (int)shots.size()) {
        finished = true;
        active = false;
        FILE* f = fopen((outputDir + "/report.txt").c_str(), "a");
        if (f) {
            fprintf(f, "TOTAL %d frames\n", frameIndex);
            fclose(f);
        }
        return;
    }

    if (!allResolved) {
        for (TrailerShot& s : shots) ResolveShot(eng, s);
        allResolved = true;
        return;
    }

    TrailerShot& shot = shots[shotIndex];

    skyTime = shot.timeOfDay / 24.0f;
    ApplyShotState(eng, shot);

    bool uiShot = shot.showInventory || shot.showQuests;
    float needSettle = uiShot ? 2.5f : 6.0f;
    int   needFrames = uiShot ? 30 : 90;

    if (settleFrames < needFrames || settleClock < needSettle) {
        ApplyCamera(shot, 0.0f, cam);
        ClearCamera(eng, cam);
        if (shot.showPlayer || shot.showInventory || shot.showQuests)
        eng.SetPlayerPosition({ (float)shot.anchorX + 0.5f, shot.anchorY + 1.0f,
                                (float)shot.anchorZ + 0.5f });
    else
        eng.SetPlayerPosition({ cam.position.x, cam.position.y, cam.position.z });
        settleClock += GetFrameTime();
        settleFrames++;
        return;
    }

    float t = shotClock / shot.duration;
    ApplyCamera(shot, t, cam);
    ClearCamera(eng, cam);
    if (shot.showPlayer || shot.showInventory || shot.showQuests)
        eng.SetPlayerPosition({ (float)shot.anchorX + 0.5f, shot.anchorY + 1.0f,
                                (float)shot.anchorZ + 0.5f });
    else
        eng.SetPlayerPosition({ cam.position.x, cam.position.y, cam.position.z });

    fadeIn = (shotClock < 0.7f) ? (shotClock / 0.7f) : 1.0f;
    float remain = shot.duration - shotClock;
    if (remain < 0.7f) fadeIn = std::max(0.0f, remain / 0.7f);

    wantCapture = true;
    shotFrames++;

    if (shotFrames == 1 || (shotFrames % 40) == 0) {
        FILE* f = fopen((outputDir + "/report.txt").c_str(), "a");
        if (f) {
            fprintf(f, "  shot %d '%s' frame %d/%d  fps=%.1f\n",
                shotIndex, shot.caption.c_str(), shotFrames,
                (int)(shot.duration * fps), 1.0f / std::max(0.001f, GetFrameTime()));
            fclose(f);
        }
    }

    shotClock += 1.0f / fps;
    if (shotClock >= shot.duration) {
        FILE* f = fopen((outputDir + "/report.txt").c_str(), "a");
        if (f) {
            fprintf(f, "%-46s %4d frames  settle %3d\n",
                shot.caption.c_str(), shotFrames, settleFrames);
            fclose(f);
        }
        shotIndex++;
        shotClock = 0.0f;
        shotFrames = 0;
        settleFrames = 0;
        settleClock = 0.0f;
    }
}

void Trailer::EndFrame() {
    if (!wantCapture) return;
    wantCapture = false;
    Capture();
}

void Trailer::Capture() {
    char path[512];
    snprintf(path, sizeof(path), "%s/frame_%05d.png", outputDir.c_str(), frameIndex);

    rlDrawRenderBatchActive();
    Image img = LoadImageFromScreen();
    ExportImage(img, path);
    UnloadImage(img);

    frameIndex++;
}

void Trailer::DrawOverlay(int sw, int sh) {
    if (!active || shotIndex >= (int)shots.size()) return;

    const TrailerShot& shot = shots[shotIndex];

    int barH = (int)(sh * 0.085f);
    DrawRectangle(0, 0, sw, barH, BLACK);
    DrawRectangle(0, sh - barH, sw, barH, BLACK);

    if (settleFrames < 30 || settleClock < 3.0f) {
        DrawRectangle(0, 0, sw, sh, BLACK);
        return;
    }

    unsigned char a = (unsigned char)(255.0f * EaseOut(fadeIn));

    if (!shot.caption.empty()) {
        int fs = (int)(sh * 0.058f);
        int w = MeasureText(shot.caption.c_str(), fs);
        int x = (sw - w) / 2;
        int y = (int)(sh * 0.70f);
        DrawText(shot.caption.c_str(), x + 3, y + 3, fs, Color{ 0, 0, 0, (unsigned char)(a * 0.7f) });
        DrawText(shot.caption.c_str(), x, y, fs, Color{ 236, 231, 221, a });
    }

    if (!shot.subcaption.empty()) {
        int fs2 = (int)(sh * 0.024f);
        int w2 = MeasureText(shot.subcaption.c_str(), fs2);
        int y2 = (int)(sh * 0.70f) + (int)(sh * 0.062f);
        DrawText(shot.subcaption.c_str(), (sw - w2) / 2, y2, fs2,
            Color{ 217, 164, 65, a });
    }

    float fadeToBlack = 1.0f - EaseOut(fadeIn);
    if (fadeToBlack > 0.001f)
        DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, (unsigned char)(255.0f * fadeToBlack) });
}
