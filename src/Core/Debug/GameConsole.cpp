#include "GameConsole.h"
#include "Logger.h"
#include "../EidosEngine.h" 
#include <algorithm>
#include <cstring>

void GameConsole::Init(Logger* loggerRef, sol::state* luaState, EidosEngine* eng) {
    this->logger = loggerRef;
    this->lua = luaState;
    this->engine = eng;

    knownCommands = {
        "gamemode 0", "gamemode 1", "tp 0 150 0",
        "fps_max 60", "fps_max 0", "render_distance 12",
        "save", "exit", "world_respawn", "give @s diamond"
    };
    logger->Log("Console initialized. Press TAB for hints.");
}

void GameConsole::Toggle() {
    isOpen = !isOpen;
    if (isOpen) {
        inputBuffer[0] = '\0'; cursorPos = 0;
        while (GetCharPressed() != 0);
    }
}

void GameConsole::Update() {
    if (IsKeyPressed(KEY_GRAVE) || IsKeyPressed(KEY_F1)) Toggle();
    if (!isOpen) return;

    int wheel = (int)GetMouseWheelMove();
    if (wheel != 0) {
        scrollOffset -= wheel;
        if (scrollOffset < 0) scrollOffset = 0;
    }

    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && key != 96) {
            size_t len = strlen(inputBuffer);
            if (len < 255) {
                inputBuffer[len] = (char)key;
                inputBuffer[len + 1] = '\0';
                cursorPos++;
                UpdateSuggestions();
            }
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && cursorPos > 0) {
        // Безопасное удаление
        size_t len = strlen(inputBuffer);
        if (len > 0) {
            // Сдвигаем все символы после курсора влево
            for (int i = cursorPos - 1; i < (int)len; i++) {
                inputBuffer[i] = inputBuffer[i + 1];
            }
            cursorPos--;
            UpdateSuggestions();
        }
    }

    if (IsKeyPressed(KEY_LEFT) && cursorPos > 0) cursorPos--;
    if (IsKeyPressed(KEY_RIGHT) && cursorPos < (int)strlen(inputBuffer)) cursorPos++;

    if (IsKeyPressed(KEY_ENTER)) {
        std::string cmd(inputBuffer);
        if (!cmd.empty()) {
            logger->Log("> " + cmd);
            commandHistory.push_back(cmd);
            historyIndex = -1;
            ExecuteCommand(cmd);
            inputBuffer[0] = '\0';
            cursorPos = 0;
            suggestions.clear();
        }
    }

    // История команд (стрелки вверх/вниз)
    if (IsKeyPressed(KEY_UP)) {
        if (!commandHistory.empty()) {
            if (historyIndex == -1) historyIndex = (int)commandHistory.size() - 1;
            else if (historyIndex > 0) historyIndex--;

            strcpy(inputBuffer, commandHistory[historyIndex].c_str());
            cursorPos = (int)strlen(inputBuffer);
        }
    }

    if (IsKeyPressed(KEY_TAB) && !suggestions.empty()) {
        strcpy(inputBuffer, suggestions[0].c_str());
        // Исправлено предупреждение C4267 (size_t -> int)
        cursorPos = (int)strlen(inputBuffer);
        suggestions.clear();
    }
}

void GameConsole::ExecuteCommand(const std::string& rawCommand) {
    if (!lua) return;

    std::stringstream ss(rawCommand);
    std::string cmd; ss >> cmd;

    std::string luaCall = cmd + "(";
    std::string arg;
    bool first = true;
    while (ss >> arg) {
        if (!first) luaCall += ", ";
        bool isNumber = !arg.empty() && std::all_of(arg.begin(), arg.end(), [](char c) { return ::isdigit(c) || c == '.' || c == '-'; });
        if (isNumber) luaCall += arg;
        else luaCall += "\"" + arg + "\"";
        first = false;
    }
    luaCall += ")";

    try {
        lua->script(luaCall);
    }
    catch (const sol::error& e) {
        logger->Log("[LUA ERR] " + std::string(e.what()));
    }
}

void GameConsole::UpdateSuggestions() {
    suggestions.clear();
    std::string input(inputBuffer);
    if (input.empty()) return;
    for (const auto& c : knownCommands) {
        if (c.find(input) == 0) suggestions.push_back(c);
    }
}

void GameConsole::Render() {
    if (!isOpen) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight() / 2;

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.9f));
    DrawRectangle(0, sh, sw, 2, GREEN);

    // Теперь GetHistory() существует в Logger.h, ошибка уйдет
    const auto& history = logger->GetHistory();
    int startY = sh - 30;
    int lineHeight = 20;

    int count = 0;
    // Отрисовка логов с учетом скролла
    int startIndex = (int)history.size() - 1 - scrollOffset;

    BeginScissorMode(0, 0, sw, sh);
    for (int i = startIndex; i >= 0; i--) {
        DrawText(history[i].c_str(), 10, startY - (count * lineHeight), 20, GREEN);
        count++;
        if ((startY - (count * lineHeight)) < 0) break;
    }

    DrawText(">", 10, sh - 25, 20, WHITE);
    DrawText(inputBuffer, 30, sh - 25, 20, WHITE);

    if ((int)(GetTime() * 2) % 2 == 0) {
        std::string sub = std::string(inputBuffer).substr(0, cursorPos);
        int w = MeasureText(sub.c_str(), 20);
        DrawRectangle(30 + w + 2, sh - 23, 10, 18, GREEN);
    }
    EndScissorMode();

    if (!suggestions.empty()) {
        int boxX = 30;
        int boxY = sh + 5;
        DrawRectangle(boxX, boxY, 300, (int)suggestions.size() * 25 + 5, Fade(BLACK, 0.8f));
        DrawRectangleLines(boxX, boxY, 300, (int)suggestions.size() * 25 + 5, GREEN);

        for (size_t i = 0; i < suggestions.size(); i++) {
            DrawText(suggestions[i].c_str(), boxX + 5, boxY + 5 + (int)i * 25, 20, LIGHTGRAY);
        }
    }
}