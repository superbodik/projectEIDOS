#include "AutoShot.h"
#include "../Core/EidosEngine.h"
#include "../World/BlockType.h"
#include "../World/WorldGenerator.h"
#include <vector>
#include <string>
#include <rlgl.h>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <algorithm>

bool AutoShot::Enabled() {
    const char* v = std::getenv("EIDOS_UISHOT");
    if (v && v[0] && v[0] != '0') return true;
    const char* b = std::getenv("EIDOS_BIOMESHOT");
    return b && b[0] && b[0] != '0';
}

void AutoShot::InitBiomeTour() {
    outputDir = "public/site/static/biomes";
    if (const char* d = std::getenv("EIDOS_BIOMESHOT_DIR")) outputDir = d;

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);

    const std::vector<std::string>& names = WorldGenerator::BiomeNames();
    for (const std::string& n : names) {
        BiomeType bt;
        if (!WorldGenerator::ParseBiome(n, bt)) continue;

        ShotStep s;
        s.name = n;
        s.biomeShot = true;
        s.biomeId = (int)bt;
        s.inventoryOpen = false;
        s.viewMode = 0;
        s.holdBlock = 0;
        s.skyTime = 0.40f;
        if (bt == BiomeType::Glacier || bt == BiomeType::SnowyTundra)
            s.skyTime = 0.44f;
        steps.push_back(s);
    }

    TraceLog(LOG_INFO, "[biomeshot] %d biomes -> %s", (int)steps.size(), outputDir.c_str());
}

void AutoShot::Init() {
    active = true;

    const char* bm = std::getenv("EIDOS_BIOMESHOT");
    if (bm && bm[0] && bm[0] != '0') {
        biomeMode = true;
        InitBiomeTour();
        return;
    }

    if (const char* d = std::getenv("EIDOS_UISHOT_DIR")) outputDir = d;

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);

    auto add = [&](const char* name, int tab, bool inv, int view, int hold) {
        ShotStep s;
        s.name = name;
        s.tab = tab;
        s.inventoryOpen = inv;
        s.viewMode = view;
        s.holdBlock = hold;
        s.equipHead = (int)BlockType::Hematite;
        s.equipChest = (int)BlockType::NativeCopper;
        s.cosmeticHat = (int)BlockType::NativeGold;
        s.cosmeticCape = (int)BlockType::Berries;
        steps.push_back(s);
        };

    add("01-first-person-empty", -1, false, 0, 0);
    add("02-first-person-block", -1, false, 0, (int)BlockType::OakPlanks);
    add("03-third-person-back", -1, false, 1, (int)BlockType::Torch);
    add("04-third-person-front", -1, false, 2, (int)BlockType::OakPlanks);
    add("05-inventory-items", 0, true, 0, (int)BlockType::OakPlanks);
    add("06-inventory-cosmetics", 1, true, 0, (int)BlockType::OakPlanks);
    add("07-inventory-nutrition", 2, true, 0, (int)BlockType::Berries);
    add("08-inventory-environment", 3, true, 0, (int)BlockType::Berries);

    ShotStep creative;
    creative.name = "09-creative";
    creative.tab = 0;
    creative.inventoryOpen = true;
    creative.viewMode = 0;
    creative.holdBlock = (int)BlockType::OakPlanks;
    creative.creative = true;
    steps.push_back(creative);

    TraceLog(LOG_INFO, "[uishot] %d shots -> %s", (int)steps.size(), outputDir.c_str());
}

void AutoShot::Apply(EidosEngine& eng, const ShotStep& s) {
    Player& p = eng.GetPlayer();

    eng.currentState = GameState::Playing;

    if (s.biomeShot) {
        eng.hudVisible = false;
        eng.SetSkyTime(s.skyTime);

        if (!placed) {
            BiomeSearchResult r = eng.worldGen.FindBiome(0, 0, (BiomeType)s.biomeId,
                26000, 128);
            anchorX = r.found ? r.x : 0;
            anchorZ = r.found ? r.z : 0;
            placed = true;
        }

        int h = eng.worldGen.GetHeight(anchorX, anchorZ);
        for (int y = h + 10; y > h - 6; --y) {
            if ((BlockType)eng.GetBlockAt(anchorX, y, anchorZ) != BlockType::Air) {
                h = y; break;
            }
        }
        const float BACK = 26.0f;
        const float LIFT = 13.0f;

        float camX = (float)anchorX - BACK * 0.90f;
        float camZ = (float)anchorZ - BACK * 0.44f;
        int groundAtCam = eng.worldGen.GetHeight((int)camX, (int)camZ);
        float py = (float)std::max(std::max(h, groundAtCam) + (int)LIFT, 74);

        eng.SetPlayerPosition({ camX, py, camZ });
        p.velocity = { 0, 0, 0 };
        p.isGrounded = true;
        p.viewMode = 0;
        p.inventory.isOpen = false;
        p.currentMode = GameMode::Spectator;

        Vector3 eye2 = p.EyePosition();
        p.camera.position = eye2;
        p.camera.target = { (float)anchorX + 0.5f, (float)h + 3.0f, (float)anchorZ + 0.5f };
        return;
    }

    eng.hudVisible = true;
    eng.SetSkyTime(0.42f);

    int gx = 148, gz = 96;
    int h = eng.worldGen.GetHeight(gx, gz);
    for (int y = h + 6; y > h - 4; --y) {
        if ((BlockType)eng.GetBlockAt(gx, y, gz) != BlockType::Air) { h = y; break; }
    }
    eng.SetPlayerPosition({ (float)gx + 0.5f, (float)(h + 1), (float)gz + 0.5f });
    p.velocity = { 0, 0, 0 };
    p.isGrounded = true;

    Vector3 eye = p.EyePosition();
    p.camera.position = eye;
    p.camera.target = { eye.x + 0.94f, eye.y - 0.12f, eye.z + 0.34f };

    p.currentMode = s.creative ? GameMode::Creative : GameMode::Survival;
    p.health = 0.8f;
    p.satiety = 0.65f;
    p.hydration = 0.55f;
    p.bodyTempC = 36.6f;

    p.viewMode = s.viewMode;
    p.inventory.isOpen = s.inventoryOpen;
    if (s.tab >= 0) p.inventory.activeTab = (Inventory::Tab)s.tab;

    p.appearance.equipment[(int)EquipSlot::Head] = s.equipHead;
    p.appearance.equipment[(int)EquipSlot::Chest] = s.equipChest;
    p.appearance.cosmetics[(int)CosmeticSlot::Hat] = s.cosmeticHat;
    p.appearance.cosmetics[(int)CosmeticSlot::Cape] = s.cosmeticCape;

    p.inventory.slots[0] = { s.holdBlock, s.holdBlock ? 12 : 0 };
    p.inventory.currentSlotIndex = 0;

    p.inventory.slots[1] = { (int)BlockType::Berries, 7 };
    p.inventory.slots[2] = { (int)BlockType::Acorn, 3 };
    p.inventory.slots[3] = { (int)BlockType::Grubs, 2 };
    p.inventory.slots[4] = { (int)BlockType::FlintPebble, 5 };
    p.inventory.slots[5] = { (int)BlockType::Granite, 31 };
    p.inventory.slots[9] = { (int)BlockType::OakLog, 22 };
    p.inventory.slots[10] = { (int)BlockType::Limestone, 40 };
    p.inventory.slots[11] = { (int)BlockType::Torch, 9 };
    p.inventory.slots[12] = { (int)BlockType::BirdEgg, 1 };
    p.inventory.slots[18] = { (int)BlockType::Marble, 17 };
}

void AutoShot::Capture(const std::string& name) {
    std::string path = outputDir + "/" + name + ".png";
    rlDrawRenderBatchActive();
    Image img = LoadImageFromScreen();
    ExportImage(img, path.c_str());
    UnloadImage(img);
    TraceLog(LOG_INFO, "[uishot] wrote %s", path.c_str());
}

void AutoShot::Update(EidosEngine& eng) {
    if (!active) return;

    if (stepIndex >= (int)steps.size()) {
        active = false;
        eng.CloseApp();
        return;
    }

    Apply(eng, steps[stepIndex]);

    waited++;
    settleClock += GetFrameTime();

    int needFrames = biomeMode ? 90 : 40;
    float needTime = biomeMode ? 4.0f : 1.2f;
    if (waited >= needFrames && settleClock >= needTime) wantCapture = true;
}

void AutoShot::EndFrame(EidosEngine& eng) {
    (void)eng;
    if (!wantCapture || stepIndex >= (int)steps.size()) return;
    wantCapture = false;

    const ShotStep& s = steps[stepIndex];
    FILE* f = fopen((outputDir + "/report.txt").c_str(), "a");
    if (f) {
        fprintf(f, "%s  want_inv=%d actual_inv=%d tab=%d view=%d state=%d hud=%d\n",
            s.name.c_str(), (int)s.inventoryOpen,
            (int)eng.GetPlayer().inventory.isOpen,
            (int)eng.GetPlayer().inventory.activeTab,
            eng.GetPlayer().viewMode,
            (int)eng.currentState,
            (int)eng.hudVisible);
        fclose(f);
    }

    Capture(steps[stepIndex].name);
    stepIndex++;
    waited = 0;
    settleClock = 0.0f;
    placed = false;
}
