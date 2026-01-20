#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <sol/sol.hpp>

class Logger; // Forward declaration
class EidosEngine;

class GameConsole {
public:
    void Init(Logger* loggerRef, sol::state* luaState, EidosEngine* engine);
    void Update();
    void Render();
    void Toggle();
    bool IsOpen() const { return isOpen; }

private:
    bool isOpen = false;
    char inputBuffer[256] = { 0 };
    int cursorPos = 0;
    int scrollOffset = 0;

    Logger* logger = nullptr;
    sol::state* lua = nullptr;
    EidosEngine* engine = nullptr;

    std::vector<std::string> commandHistory;
    int historyIndex = -1;
    std::vector<std::string> knownCommands;
    std::vector<std::string> suggestions;
    int suggestionIndex = -1;

    void ExecuteCommand(const std::string& cmd);
    void UpdateSuggestions();
};