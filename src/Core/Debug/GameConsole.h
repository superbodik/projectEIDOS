#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <sol/sol.hpp>

class Logger;
class EidosEngine;

class GameConsole {
public:
    void Init(Logger* loggerRef, sol::state* luaState, EidosEngine* engine);
    void Update();
    void Render();
    void Toggle();
    bool IsOpen() const { return isOpen; }

private:
    static const int MAX_INPUT = 255;
    static const int SUGGEST_ROWS = 8;
    static const int SUGGEST_ROW_H = 25;
    static const int SUGGEST_X = 30;
    static const int SUGGEST_W = 460;

    bool isOpen = false;
    char inputBuffer[256] = { 0 };
    int cursorPos = 0;
    int scrollOffset = 0;

    Logger* logger = nullptr;
    sol::state* lua = nullptr;
    EidosEngine* engine = nullptr;

    std::vector<std::string> commandHistory;
    int historyIndex = -1;
    std::string draft;

    std::vector<std::string> knownCommands;
    std::vector<std::string> suggestions;
    int suggestionIndex = -1;
    int suggestionScroll = 0;
    int hoverIndex = -1;

    int repeatKey = 0;
    float repeatTimer = 0.0f;

    void ExecuteCommand(const std::string& cmd);
    void UpdateSuggestions();
    void BuildCommandList();

    void SetInput(const std::string& text);
    void AcceptSuggestion(int index);
    void HistoryStep(int delta);
    void MoveSuggestion(int delta);

    bool KeyRepeat(int key);
    int ConsoleHeight() const { return GetScreenHeight() / 2; }
    int VisibleSuggestions() const;
    Rectangle SuggestionBounds() const;
    int SuggestionAtPoint(Vector2 p) const;
};
