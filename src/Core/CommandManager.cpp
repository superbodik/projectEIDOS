#include "CommandManager.h"
#include "EidosEngine.h" 
#include "../World/Chunk.h" 
#include "../World/BlockType.h" 

#include <string>
#include <vector>
#include <algorithm>

CommandManager::CommandManager(EidosEngine* enginePtr) : engine(enginePtr) {}

void CommandManager::BindCommands(sol::state& lua) {

    lua.set_function("gamemode", &CommandManager::Cmd_Gamemode, this);
    lua.set_function("gm", &CommandManager::Cmd_Gamemode, this);
    lua.set_function("tp", &CommandManager::Cmd_Teleport, this);

    lua.set_function("give", &CommandManager::Cmd_Give, this);

    lua.set_function("fps_max", [this](int fps) {
        engine->SetMaxFPS(fps);
        std::string msg = (fps == 0) ? "FPS Limit: Unlimited" : "FPS Limit: " + std::to_string(fps);
        engine->debugSystem.Log(msg); 
        });

    sol::table settings = lua.create_named_table("settings");
    settings.set_function("chunkrender", [this](int dist) {
        engine->SetRenderDistance(dist);
        engine->debugSystem.Log("Render distance set to " + std::to_string(dist)); 
        });

    lua.set_function("world_respawn", [this](sol::optional<int> seed) {
        int newSeed = seed.value_or(GetRandomValue(0, 999999));
        engine->worldGen.SetSeed(newSeed);

        {
            engine->chunks.clear();
        }

        engine->GetPlayer().position = { 0.5f, 150.0f, 0.5f };
        engine->GetPlayer().velocity = { 0, 0, 0 };
        engine->debugSystem.Log("World regenerated! Seed: " + std::to_string(newSeed)); 
        });
}

void CommandManager::Cmd_Gamemode(sol::object value) {
    int modeIndex = 0;

    if (value.is<int>()) {
        modeIndex = value.as<int>();
    }
    else if (value.is<std::string>()) {
        std::string s = value.as<std::string>();
        if (s == "c" || s == "creative" || s == "1") modeIndex = 1;
        else if (s == "sp" || s == "spectator" || s == "2") modeIndex = 2;
        else modeIndex = 0;
    }

    std::string modeLog = "Survival";
    if (modeIndex == 1) modeLog = "Creative";
    if (modeIndex == 2) modeLog = "Spectator";

    engine->GetPlayer().SetGameMode(modeIndex);
    engine->debugSystem.Log("Gamemode set to " + modeLog);
}

void CommandManager::Cmd_Teleport(float x, float y, float z) {
    auto& player = engine->GetPlayer();
    player.position = { x, y, z };
    player.camera.position = player.position;

    std::string msg = "Teleported to " + std::to_string((int)x) + ", " +
        std::to_string((int)y) + ", " + std::to_string((int)z);
    engine->debugSystem.Log(msg); // Исправлено
}

void CommandManager::Cmd_Give(std::string selector, sol::object blockIdent, sol::optional<int> amount) {
    if (selector != "@s" && selector != "@a" && selector != "@p" && selector != "me") {
        engine->debugSystem.Log("[Err] Unknown selector: " + selector); // Исправлено
        return;
    }

    int blockID = 0;
    std::string blockName = "";

    if (blockIdent.is<int>()) {
        blockID = blockIdent.as<int>();
        blockName = "Block #" + std::to_string(blockID);
    }
    else if (blockIdent.is<std::string>()) {
        std::string name = blockIdent.as<std::string>();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        blockID = GetBlockIDByName(name);
        blockName = name;
    }

    int count = amount.value_or(64);
    if (count <= 0) count = 1;

    if (blockID > 0) {
        bool success = engine->GetPlayer().AddItem(blockID, count);
        if (success) {
            engine->debugSystem.Log("Given " + std::to_string(count) + " " + blockName + " to " + selector); // Исправлено
        }
        else {
            engine->debugSystem.Log("[Err] Inventory is full"); // Исправлено
        }
    }
    else {
        engine->debugSystem.Log("[Err] Unknown block or ID 0"); // Исправлено
    }
}

int CommandManager::GetBlockIDByName(std::string name) {
    if (name == "water") return (int)BlockType::Water;
    if (name == "lava") return (int)BlockType::Lava;
    if (name == "bedrock") return (int)BlockType::Bedrock;

    if (name == "grass") return (int)BlockType::Grass;
    if (name == "dirt") return (int)BlockType::Dirt;
    if (name == "coarse_dirt") return (int)BlockType::CoarseDirt;
    if (name == "mud") return (int)BlockType::Mud;
    if (name == "clay") return (int)BlockType::Clay;
    if (name == "sand") return (int)BlockType::Sand;
    if (name == "red_sand") return (int)BlockType::RedSand;
    if (name == "gravel") return (int)BlockType::Gravel;

    if (name == "stone" || name == "granite") return (int)BlockType::Granite;
    if (name == "basalt") return (int)BlockType::Basalt;
    if (name == "limestone") return (int)BlockType::Limestone;
    if (name == "marble") return (int)BlockType::Marble;

    if (name == "snow") return (int)BlockType::Snow;
    if (name == "ice") return (int)BlockType::Ice;

    if (name == "log" || name == "oak_log") return (int)BlockType::OakLog;
    if (name == "leaves" || name == "oak_leaves") return (int)BlockType::OakLeaves;
    if (name == "spruce_log") return (int)BlockType::SpruceLog;
    if (name == "spruce_leaves") return (int)BlockType::SpruceLeaves;
    if (name == "cactus") return (int)BlockType::Cactus;

    if (name == "coal_ore") return (int)BlockType::BituminousCoal;
    if (name == "iron_ore") return (int)BlockType::Hematite;
    if (name == "gold_ore") return (int)BlockType::NativeGold;
    if (name == "copper_ore") return (int)BlockType::NativeCopper;

    return 0;
}