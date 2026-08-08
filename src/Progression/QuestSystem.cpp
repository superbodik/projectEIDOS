#include "QuestSystem.h"
#include "../Core/EidosEngine.h"
#include "../Inventory/BlockInfo.h"
#include <fstream>
#include <algorithm>
#include <set>

QuestSystem::QuestSystem() {
    BuildQuests();
    done.assign(eras[0].quests.size(), false);
}

void QuestSystem::Toggle() { isOpen = !isOpen; }

void QuestSystem::Reset() {
    done.assign(eras[0].quests.size(), false);
    selected = 0;
    scroll = 0;
}

void QuestSystem::BuildQuests() {
    Era stone;
    stone.name = "I. Primal Chaos";
    stone.version = "now";
    stone.playable = true;

    auto collect = [](const char* id, const char* title, const char* desc, int block, int n) {
        Quest q; q.id = id; q.title = title; q.desc = desc;
        q.kind = QuestKind::Collect; q.blockId = block; q.amount = n;
        return q;
        };
    auto anyOf = [](const char* id, const char* title, const char* desc,
        std::vector<int> ids, int n) {
            Quest q; q.id = id; q.title = title; q.desc = desc;
            q.kind = QuestKind::CollectAny; q.anyOf = std::move(ids); q.amount = n;
            return q;
        };
    auto distinct = [](const char* id, const char* title, const char* desc,
        std::vector<int> ids, int n) {
            Quest q; q.id = id; q.title = title; q.desc = desc;
            q.kind = QuestKind::CollectDistinct; q.anyOf = std::move(ids); q.amount = n;
            return q;
        };
    auto biome = [](const char* id, const char* title, const char* desc, const char* name) {
        Quest q; q.id = id; q.title = title; q.desc = desc;
        q.kind = QuestKind::ReachBiome; q.biome = name;
        return q;
        };
    auto depth = [](const char* id, const char* title, const char* desc, int y) {
        Quest q; q.id = id; q.title = title; q.desc = desc;
        q.kind = QuestKind::ReachDepth; q.amount = y;
        return q;
        };
    auto height = [](const char* id, const char* title, const char* desc, int y) {
        Quest q; q.id = id; q.title = title; q.desc = desc;
        q.kind = QuestKind::ReachHeight; q.amount = y;
        return q;
        };

    stone.quests = {
        collect("dirt",    "First Soil",      "Dig 4 dirt blocks. LMB breaks a block; in survival it drops into your inventory.", 7, 4),
        collect("logs",    "Lumberjack",      "Fell a tree: gather 8 oak logs.", 100, 8),
        anyOf("stone",     "Stonecutter",     "Mine 16 blocks of bedrock stone. Which rock you find depends on the geological province you stand in - dig a cliff and see.",
              { 20,21,22,23,24,25,26,27,28,30,31,32,35,36,37,38,40,41,42,43,44,45 }, 16),
        anyOf("pebble",    "Reading the Land","Pick up 3 surface pebbles. A pebble is the rock beneath your feet - the land tells you what it is made of.",
              { 130,136,137,138,139,140 }, 3),
        collect("forage",  "Forager",         "Gather 3 plant fibres from tall grass. Everything later starts with cordage.", 154, 3),
        anyOf("firstfood", "First Meal",      "Eat something. Right-click food in hand. Berries grow on bushes in forests, acorns fall from oak leaves.",
              { 150,151,152,153,115 }, 1),
        biome("river",     "To the River",    "Find a river. The command 'locate biome River' points the way.", "River"),
        collect("reed",    "Reeds",           "Gather 4 reeds on a riverbank - useful for baskets.", 126, 4),
        distinct("diet",   "Balanced Diet",   "Hold three different nutrient groups at once: a fruit, a grain and a protein. Living on one group caps how far your health can recover.",
              { 150,151,152,153 }, 3),
        collect("coalpeb", "Coal Trace",      "Find a coal pebble - a coal seam sleeps below it.", 133, 1),
        collect("flint",   "Sharp Edge",      "Find flint. Chert and shale weather into it. Two flint stones struck together will one day give you a blade.", 136, 2),
        collect("torch",   "First Fire",      "Get 4 torches. Never enter a cave without light - the dark below is total.", 127, 4),
        depth("caves",     "Into the Dark",   "Descend into caves below y=40. Place torches behind you so you can find the way out.", 40),
        collect("coal",    "Coal",            "Mine 8 bituminous coal - fuel for the furnaces to come.", 56, 8),
        collect("copper",  "Copper Gleam",    "Mine 4 native copper. It lives in basalt and andesite - find a volcanic province. Copper pebbles on the surface mark it.", 50, 4),
        depth("deep",      "The Deep",        "Reach depth y=20. Rock is harder here, ores are richer.", 20),
        collect("iron",    "Signs of Iron",   "Mine 4 hematite. Shale, chert and schist carry it; a sedimentary basin is the safest bet.", 53, 4),
        biome("mount",     "The Heights",     "Climb into a mountain biome.", "Mountains"),
        height("peak",     "Above the Clouds","Reach altitude y=130.", 130),
        collect("sapling", "Seed of Forests", "Obtain an oak sapling. Oaks drop acorns that grow into young trees.", 123, 1),
        collect("gold",    "Gold!",           "Mine native gold. It rides quartz veins through schist and gneiss - a fold belt, below y=46.", 60, 1),
        biome("snow",      "Polar Explorer",  "Reach the snowy tundra. Walk straight north or south - climate follows latitude.", "SnowyTundra"),
        collect("nuggets",  "Pieces of Copper", "Gather 12 copper nuggets by breaking copper pebbles on the ground. Copper is too soft to forge - it can only be melted and cast. Every tool of Era II starts as a pile of these.", 155, 12),
        collect("clay",     "Wet Clay",        "Dig 8 clay lumps from a riverbank. Clay is the other half of Era II: without a crucible there is nothing to melt copper in.", 160, 8),
        collect("crucible", "The Crucible",    "Shape an unfired crucible: hold clay lumps and right-click (4 lumps). Hold CTRL while right-clicking to shape a pickaxe mould instead (3 lumps). Wet clay still has to be pit-fired.", 161, 1),
        collect("diamond", "Kimberlite",      "Mine kimberlite below y=22. It exists only under a shield province - the oldest crust there is. Diamond pebbles on the surface are the only clue. The finale of the Stone Age.", 66, 1),
    };
    eras.push_back(stone);

    auto teaser = [this](const char* name, const char* ver, std::initializer_list<const char*> titles) {
        Era e; e.name = name; e.version = ver; e.playable = false;
        for (const char* t : titles) { Quest q; q.title = t; e.quests.push_back(q); }
        eras.push_back(e);
        };

    teaser("II. Clay & Casting", "v0.9.5", { "Clay Pot", "Pit Kiln", "Crucible & Bellows", "Cast a Copper Ingot", "Copper Pickaxe", "First Crops" });
    teaser("III. Kinematics", "v1.0", { "Bronze Alloy 9:1", "The Anvil", "Water Wheel", "Ore Crusher", "Mill & Bread" });
    teaser("IV. Thermodynamics", "v1.1", { "Bloomery", "Bloom & Slag", "Steel", "Prospecting Drill", "Mine Supports", "Full Armor" });
    teaser("V. Infrastructure", "v1.2", { "Gunpowder", "Windmill", "Plough & Horse", "Blueprint Book", "Counterweight Lift" });
    teaser("VI. Steam Engine", "v1.3", { "Steam Boiler", "Safety Valve", "Conveyor Belt", "Drilling Rig", "Hands-free Cycle", "Railroad" });
    teaser("VII. EIDOS Grid", "v1.4", { "Waterfall Dam", "Power Grid", "Electrolysis", "Relay Logic", "Auto-shutdown" });
    teaser("VIII. Information", "v1.5", { "Microprocessor", "First Lua Script", "Robotic Arm", "Courier Drones", "Geo-scanner", "Scripted Reactor" });
    teaser("IX. Orbital Vector", "v2.0", { "Launch Pad", "Rocket Fuel", "First Satellite", "Moon Landing", "Autonomous Outpost" });
    teaser("X. EIDOS Absolute", "v2.5", { "Nanofabricator", "Quantum Portal", "AI Core", "Terraforming", "Monument to the First Knife" });
}

int QuestSystem::ActiveIndex() const {
    for (size_t i = 0; i < done.size(); i++)
        if (!done[i]) return (int)i;
    return -1;
}

int QuestSystem::CompletedCount() const {
    int n = 0;
    for (bool b : done) if (b) n++;
    return n;
}

bool QuestSystem::CheckQuest(EidosEngine& eng, const Quest& q, int& progress, int& needed) {
    Player& p = eng.GetPlayer();
    switch (q.kind) {
    case QuestKind::Collect:
        progress = p.inventory.CountItem(q.blockId);
        needed = q.amount;
        return progress >= needed;
    case QuestKind::CollectAny: {
        progress = 0;
        for (int id : q.anyOf) progress += p.inventory.CountItem(id);
        needed = q.amount;
        return progress >= needed;
    }
    case QuestKind::CollectDistinct: {
        progress = 0;
        for (int id : q.anyOf) if (p.inventory.CountItem(id) > 0) progress++;
        needed = q.amount;
        return progress >= needed;
    }
    case QuestKind::ReachBiome: {
        progress = 0; needed = 1;
        std::string here = eng.worldGen.GetBiomeName((int)p.position.x, (int)p.position.z);
        BiomeType want, got;
        if (!WorldGenerator::ParseBiome(q.biome, want)) return false;
        if (!WorldGenerator::ParseBiome(here, got)) return false;
        if (want == got) { progress = 1; return true; }
        return false;
    }
    case QuestKind::ReachDepth:
        progress = (p.position.y <= (float)q.amount) ? 1 : 0;
        needed = 1;
        return progress == 1;
    case QuestKind::ReachHeight:
        progress = (p.position.y >= (float)q.amount) ? 1 : 0;
        needed = 1;
        return progress == 1;
    }
    return false;
}

void QuestSystem::Update(EidosEngine& eng, float dt) {
    if (toastTimer > 0.0f) toastTimer -= dt;

    checkTimer += dt;
    if (checkTimer < 0.5f) return;
    checkTimer = 0.0f;

    int active = ActiveIndex();
    if (active < 0) return;

    int prog = 0, need = 1;
    if (CheckQuest(eng, MainQuests()[active], prog, need)) {
        done[active] = true;
        toastText = "Quest complete: " + MainQuests()[active].title;
        toastTimer = 4.0f;
        eng.debugSystem.Log("[QUEST] " + MainQuests()[active].title + " — done!");
        Save("saves/" + eng.currentWorldName);
        int next = ActiveIndex();
        if (next >= 0) selected = next;
    }
}

void QuestSystem::Load(const std::string& worldPath) {
    Reset();
    std::ifstream in(worldPath + "/quests.txt");
    if (!in) return;
    std::set<std::string> ids;
    std::string line;
    while (std::getline(in, line)) if (!line.empty()) ids.insert(line);
    for (size_t i = 0; i < MainQuests().size(); i++)
        if (ids.count(MainQuests()[i].id)) done[i] = true;
    int active = ActiveIndex();
    selected = (active >= 0) ? active : (int)MainQuests().size() - 1;
}

void QuestSystem::Save(const std::string& worldPath) const {
    std::ofstream out(worldPath + "/quests.txt", std::ios::trunc);
    if (!out) return;
    for (size_t i = 0; i < MainQuests().size(); i++)
        if (done[i]) out << MainQuests()[i].id << "\n";
}

void QuestSystem::Draw(EidosEngine& eng, int sw, int sh) {
    if (toastTimer > 0.0f && !isOpen) {
        int w = MeasureText(toastText.c_str(), 22) + 40;
        int x = sw / 2 - w / 2;
        DrawRectangle(x, 60, w, 40, Fade(BLACK, 0.72f));
        DrawRectangleLines(x, 60, w, 40, GOLD);
        DrawText(toastText.c_str(), x + 20, 69, 22, GOLD);
    }
    if (!isOpen) return;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.78f));

    int panelW = std::min(1060, sw - 60);
    int panelH = std::min(640, sh - 60);
    int px = (sw - panelW) / 2;
    int py = (sh - panelH) / 2;
    DrawRectangle(px, py, panelW, panelH, Color{ 22, 26, 34, 245 });
    DrawRectangleLines(px, py, panelW, panelH, Color{ 90, 110, 140, 255 });

    int total = (int)MainQuests().size();
    int doneN = CompletedCount();
    std::string head = TextFormat("EIDOS NEURAL NET    [%d/%d]  Era I - %d%%", doneN, total, doneN * 100 / total);
    DrawText(head.c_str(), px + 20, py + 14, 24, RAYWHITE);
    DrawText("L / Esc - close, wheel - scroll, click - select", px + 20, py + 42, 14, GRAY);

    int eraW = 220;
    int eraY = py + 70;
    for (size_t e = 0; e < eras.size(); e++) {
        int ey = eraY + (int)e * 30;
        if (ey > py + panelH - 30) break;
        bool cur = (e == 0);
        Color c = cur ? GOLD : Color{ 110, 116, 128, 255 };
        DrawText(eras[e].name.c_str(), px + 16, ey, 17, c);
        if (!cur) DrawText(eras[e].version.c_str(), px + eraW - 46, ey, 15, Color{ 80, 86, 96, 255 });
    }
    DrawLine(px + eraW, py + 66, px + eraW, py + panelH - 12, Color{ 60, 70, 88, 255 });

    int listX = px + eraW + 16;
    int listW = 420;
    int rowH = 30;
    int listTop = py + 70;
    int visRows = (panelH - 90) / rowH;

    int wheel = (int)GetMouseWheelMove();
    if (wheel != 0) scroll = std::clamp(scroll - wheel, 0, std::max(0, total - visRows));

    int active = ActiveIndex();
    Vector2 mouse = GetMousePosition();

    for (int r = 0; r < visRows; r++) {
        int qi = scroll + r;
        if (qi >= total) break;
        int rowY = listTop + r * rowH;
        Rectangle rowRect = { (float)listX, (float)rowY, (float)listW, (float)rowH - 3 };

        bool hovered = CheckCollisionPointRec(mouse, rowRect);
        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selected = qi;

        bool isDone = done[qi];
        bool isActive = (qi == active);
        bool locked = !isDone && !isActive;

        if (qi == selected) DrawRectangleRec(rowRect, Fade(SKYBLUE, 0.16f));
        else if (hovered) DrawRectangleRec(rowRect, Fade(WHITE, 0.06f));

        if (qi > 0) DrawLine(listX + 11, rowY - 3, listX + 11, rowY + 6, Color{ 70, 80, 96, 255 });

        Color dot = isDone ? GREEN : (isActive ? GOLD : Color{ 84, 90, 100, 255 });
        DrawCircle(listX + 11, rowY + 12, 6.0f, dot);
        if (isDone) DrawText("v", listX + 7, rowY + 4, 16, Color{ 10, 40, 16, 255 });

        Color txt = isDone ? Color{ 150, 210, 150, 255 } : (isActive ? RAYWHITE : Color{ 120, 126, 136, 255 });
        std::string title = MainQuests()[qi].title;
        if (locked) title = "??? " + title;
        DrawText(title.c_str(), listX + 28, rowY + 5, 19, txt);

        if (isActive) {
            int prog = 0, need = 1;
            CheckQuest(eng, MainQuests()[qi], prog, need);
            std::string pr = TextFormat("%d/%d", std::min(prog, need), need);
            DrawText(pr.c_str(), listX + listW - MeasureText(pr.c_str(), 17) - 6, rowY + 6, 17, GOLD);
        }
    }

    int infoX = listX + listW + 16;
    int infoW = px + panelW - infoX - 16;
    if (infoW > 120 && selected >= 0 && selected < total) {
        const Quest& q = MainQuests()[selected];
        DrawText(q.title.c_str(), infoX, listTop, 21, RAYWHITE);

        int ty = listTop + 34;
        std::string text = q.desc;
        while (!text.empty() && ty < py + panelH - 60) {
            size_t cut = text.size();
            while (MeasureText(text.substr(0, cut).c_str(), 16) > infoW && cut > 8) {
                size_t sp = text.rfind(' ', cut - 2);
                cut = (sp == std::string::npos || sp < 4) ? cut - 4 : sp;
            }
            DrawText(text.substr(0, cut).c_str(), infoX, ty, 16, Color{ 190, 196, 206, 255 });
            text = (cut < text.size()) ? text.substr(text[cut] == ' ' ? cut + 1 : cut) : "";
            ty += 21;
        }

        int prog = 0, need = 1;
        CheckQuest(eng, q, prog, need);
        int barY = py + panelH - 44;
        DrawRectangle(infoX, barY, infoW, 18, Color{ 40, 46, 58, 255 });
        float frac = done[selected] ? 1.0f : std::clamp((float)prog / (float)need, 0.0f, 1.0f);
        DrawRectangle(infoX, barY, (int)(infoW * frac), 18, done[selected] ? GREEN : GOLD);
        std::string bt = done[selected] ? "Complete" : TextFormat("%d / %d", std::min(prog, need), need);
        DrawText(bt.c_str(), infoX + infoW / 2 - MeasureText(bt.c_str(), 15) / 2, barY + 2, 15, BLACK);
    }
}
