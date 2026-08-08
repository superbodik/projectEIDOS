#pragma once
#include <sol/sol.hpp>
#include <string>

class EidosEngine;

class CommandManager {
public:
    CommandManager(EidosEngine* engine);
    void BindCommands(sol::state& lua);

private:
    EidosEngine* engine;

    void Cmd_Gamemode(sol::object value);
    void Cmd_Teleport(float x, float y, float z);
    void Cmd_Give(std::string selector, sol::object blockIdent, sol::optional<int> amount);
    int GetBlockIDByName(std::string name);
};
