#include "GameConsole.h"
#include "Logger.h"
#include "../EidosEngine.h"
#include <algorithm>
#include <cstring>
#include <cctype>

void GameConsole::BuildCommandList() {
    knownCommands = {
        "gamemode 0", "gamemode 1", "gamemode 2",
        "gm 0", "gm 1", "gm 2",
        "tp 0 150 0",
        "give @s dirt 64",
        "fps_max 60", "fps_max 165", "fps_max 0",
        "settings.chunkrender 8", "settings.chunkrender 12", "settings.chunkrender 16",
        "world_respawn",
        "wind 0", "wind 5", "wind 12", "wind 25", "wind auto",
        "save", "exit",
        "time set day", "time set night", "time set noon",
        "time set midnight", "time set sunrise", "time set sunset",
        "locate biome list",
        "geology",
        "gamerule randomTickSpeed", "gamerule randomTickSpeed 3",
        "gamerule randomTickSpeed 100", "gamerule randomTickSpeed 1000",
        "gamerule randomTickSpeed 0"
    };
    for (const std::string& n : WorldGenerator::BiomeNames())
        knownCommands.push_back("locate biome " + n);
}

void GameConsole::Init(Logger* loggerRef, sol::state* luaState, EidosEngine* eng) {
    this->logger = loggerRef;
    this->lua = luaState;
    this->engine = eng;
    BuildCommandList();
    logger->Log("Console ready (~). TAB completes, UP/DOWN history, F1 hides HUD.");
}

void GameConsole::Toggle() {
    isOpen = !isOpen;
    if (isOpen) {
        inputBuffer[0] = '\0';
        cursorPos = 0;
        suggestions.clear();
        suggestionIndex = -1;
        suggestionScroll = 0;
        historyIndex = -1;
        draft.clear();
        repeatKey = 0;
        while (GetCharPressed() != 0);
    }
}

bool GameConsole::KeyRepeat(int key) {
    if (IsKeyPressed(key)) {
        repeatKey = key;
        repeatTimer = 0.0f;
        return true;
    }
    if (repeatKey == key && IsKeyDown(key)) {
        repeatTimer += GetFrameTime();
        if (repeatTimer >= 0.45f) {
            repeatTimer -= 0.045f;
            return true;
        }
    }
    if (repeatKey == key && !IsKeyDown(key)) repeatKey = 0;
    return false;
}

int GameConsole::VisibleSuggestions() const {
    int n = (int)suggestions.size();
    return n < SUGGEST_ROWS ? n : SUGGEST_ROWS;
}

Rectangle GameConsole::SuggestionBounds() const {
    int rows = VisibleSuggestions();
    return { (float)SUGGEST_X, (float)(ConsoleHeight() + 5),
             (float)SUGGEST_W, (float)(rows * SUGGEST_ROW_H + 6) };
}

int GameConsole::SuggestionAtPoint(Vector2 p) const {
    if (suggestions.empty()) return -1;
    Rectangle b = SuggestionBounds();
    if (!CheckCollisionPointRec(p, b)) return -1;
    int row = (int)((p.y - (b.y + 3)) / SUGGEST_ROW_H);
    if (row < 0 || row >= VisibleSuggestions()) return -1;
    int idx = suggestionScroll + row;
    if (idx < 0 || idx >= (int)suggestions.size()) return -1;
    return idx;
}

void GameConsole::SetInput(const std::string& text) {
    std::string t = text.substr(0, MAX_INPUT);
    memcpy(inputBuffer, t.c_str(), t.size() + 1);
    cursorPos = (int)t.size();
}

void GameConsole::AcceptSuggestion(int index) {
    if (index < 0 || index >= (int)suggestions.size()) return;
    SetInput(suggestions[index]);
    suggestions.clear();
    suggestionIndex = -1;
    suggestionScroll = 0;
}

void GameConsole::MoveSuggestion(int delta) {
    int n = (int)suggestions.size();
    if (n == 0) return;
    if (suggestionIndex < 0) suggestionIndex = (delta > 0) ? 0 : n - 1;
    else suggestionIndex = (suggestionIndex + delta + n) % n;

    if (suggestionIndex < suggestionScroll) suggestionScroll = suggestionIndex;
    int rows = VisibleSuggestions();
    if (suggestionIndex >= suggestionScroll + rows) suggestionScroll = suggestionIndex - rows + 1;
    int maxScroll = n - rows;
    if (maxScroll < 0) maxScroll = 0;
    if (suggestionScroll > maxScroll) suggestionScroll = maxScroll;
    if (suggestionScroll < 0) suggestionScroll = 0;
}

void GameConsole::HistoryStep(int delta) {
    if (commandHistory.empty()) return;
    int n = (int)commandHistory.size();

    if (historyIndex == -1) {
        if (delta > 0) return;
        draft = inputBuffer;
        historyIndex = n - 1;
    }
    else {
        int next = historyIndex - delta;
        if (next >= n) {
            historyIndex = -1;
            SetInput(draft);
            suggestions.clear();
            suggestionIndex = -1;
            return;
        }
        if (next < 0) next = 0;
        historyIndex = next;
    }
    SetInput(commandHistory[historyIndex]);
    suggestions.clear();
    suggestionIndex = -1;
}

void GameConsole::Update() {
    if (IsKeyPressed(KEY_GRAVE)) Toggle();
    if (!isOpen) return;

    Vector2 mouse = GetMousePosition();
    hoverIndex = SuggestionAtPoint(mouse);

    int wheel = (int)GetMouseWheelMove();
    if (wheel != 0) {
        if (hoverIndex >= 0 || CheckCollisionPointRec(mouse, SuggestionBounds())) {
            int maxScroll = (int)suggestions.size() - VisibleSuggestions();
            if (maxScroll < 0) maxScroll = 0;
            suggestionScroll = std::clamp(suggestionScroll - wheel, 0, maxScroll);
        }
        else {
            scrollOffset -= wheel;
            if (scrollOffset < 0) scrollOffset = 0;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverIndex >= 0) {
        AcceptSuggestion(hoverIndex);
        return;
    }

    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && key != 96) {
            int len = (int)strlen(inputBuffer);
            if (len < MAX_INPUT) {
                for (int i = len; i >= cursorPos; i--) inputBuffer[i + 1] = inputBuffer[i];
                inputBuffer[cursorPos] = (char)key;
                cursorPos++;
                historyIndex = -1;
                UpdateSuggestions();
            }
        }
        key = GetCharPressed();
    }

    if (KeyRepeat(KEY_BACKSPACE) && cursorPos > 0) {
        int len = (int)strlen(inputBuffer);
        for (int i = cursorPos - 1; i < len; i++) inputBuffer[i] = inputBuffer[i + 1];
        cursorPos--;
        historyIndex = -1;
        UpdateSuggestions();
    }

    if (KeyRepeat(KEY_DELETE) && cursorPos < (int)strlen(inputBuffer)) {
        int len = (int)strlen(inputBuffer);
        for (int i = cursorPos; i < len; i++) inputBuffer[i] = inputBuffer[i + 1];
        historyIndex = -1;
        UpdateSuggestions();
    }

    if (KeyRepeat(KEY_LEFT) && cursorPos > 0) cursorPos--;
    if (KeyRepeat(KEY_RIGHT) && cursorPos < (int)strlen(inputBuffer)) cursorPos++;
    if (IsKeyPressed(KEY_HOME)) cursorPos = 0;
    if (IsKeyPressed(KEY_END)) cursorPos = (int)strlen(inputBuffer);

    if (KeyRepeat(KEY_UP)) {
        if (!suggestions.empty()) MoveSuggestion(-1);
        else HistoryStep(-1);
    }
    if (KeyRepeat(KEY_DOWN)) {
        if (!suggestions.empty()) MoveSuggestion(1);
        else HistoryStep(1);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (!suggestions.empty()) {
            suggestions.clear();
            suggestionIndex = -1;
            suggestionScroll = 0;
        }
        else Toggle();
        return;
    }

    if (IsKeyPressed(KEY_TAB) && !suggestions.empty()) {
        AcceptSuggestion(suggestionIndex >= 0 ? suggestionIndex : 0);
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (suggestionIndex >= 0 && !suggestions.empty()) {
            AcceptSuggestion(suggestionIndex);
            return;
        }
        std::string cmd(inputBuffer);
        if (!cmd.empty()) {
            logger->Log("> " + cmd);
            if (commandHistory.empty() || commandHistory.back() != cmd)
                commandHistory.push_back(cmd);
            historyIndex = -1;
            draft.clear();
            ExecuteCommand(cmd);
            inputBuffer[0] = '\0';
            cursorPos = 0;
            suggestions.clear();
            suggestionIndex = -1;
            suggestionScroll = 0;
            scrollOffset = 0;
        }
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
        auto isNumberCheck = [&arg](char c, size_t idx) { return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || (c == '-' && idx == 0); };
        bool isNumber = !arg.empty();
        for (size_t i = 0; i < arg.size(); i++) { if (!isNumberCheck(arg[i], i)) { isNumber = false; break; } }
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
    suggestionIndex = -1;
    suggestionScroll = 0;

    std::string input(inputBuffer);
    if (input.empty()) return;

    std::string lowered = input;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });

    std::vector<std::string> partial;
    for (const std::string& c : knownCommands) {
        std::string lc = c;
        std::transform(lc.begin(), lc.end(), lc.begin(),
            [](unsigned char ch) { return (char)std::tolower(ch); });
        if (lc.rfind(lowered, 0) == 0) suggestions.push_back(c);
        else if (lc.find(lowered) != std::string::npos) partial.push_back(c);
    }
    suggestions.insert(suggestions.end(), partial.begin(), partial.end());
    if (suggestions.size() == 1 && suggestions[0] == input) suggestions.clear();
}

void GameConsole::Render() {
    if (!isOpen) return;

    int sw = GetScreenWidth();
    int sh = ConsoleHeight();

    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.9f));
    DrawRectangle(0, sh, sw, 2, GREEN);

    const auto& history = logger->GetHistory();

    int fontSize = 20;
    int lineHeight = 24;
    int startY = sh - 35;

    int startIndex = (int)history.size() - 1 - scrollOffset;
    if (startIndex < 0) startIndex = 0;
    if (startIndex >= (int)history.size()) startIndex = (int)history.size() - 1;

    BeginScissorMode(0, 0, sw, sh);

    int currentY = startY;
    for (int i = startIndex; i >= 0; i--) {
        DrawText(history[i].c_str(), 10, currentY, fontSize, GREEN);
        currentY -= lineHeight;
        if (currentY < -20) break;
    }

    DrawText(">", 10, sh - 25, 20, WHITE);
    DrawText(inputBuffer, 30, sh - 25, 20, WHITE);

    if ((int)(GetTime() * 2) % 2 == 0) {
        std::string sub = std::string(inputBuffer).substr(0, cursorPos);
        int w = MeasureText(sub.c_str(), 20);
        DrawRectangle(30 + w + 2, sh - 23, 10, 18, GREEN);
    }
    EndScissorMode();

    if (suggestions.empty()) return;

    Rectangle b = SuggestionBounds();
    int rows = VisibleSuggestions();

    DrawRectangleRec(b, Fade(BLACK, 0.85f));
    DrawRectangleLinesEx(b, 1, GREEN);

    for (int r = 0; r < rows; r++) {
        int idx = suggestionScroll + r;
        if (idx >= (int)suggestions.size()) break;
        int rowY = (int)b.y + 3 + r * SUGGEST_ROW_H;

        bool selected = (idx == suggestionIndex);
        bool hovered = (idx == hoverIndex);
        if (selected || hovered)
            DrawRectangle((int)b.x + 1, rowY, (int)b.width - 2, SUGGEST_ROW_H,
                Fade(GREEN, selected ? 0.30f : 0.15f));

        DrawText(suggestions[idx].c_str(), (int)b.x + 6, rowY + 3, 20,
            selected ? WHITE : LIGHTGRAY);
    }

    if ((int)suggestions.size() > rows) {
        std::string more = TextFormat("%d/%d", suggestionIndex >= 0 ? suggestionIndex + 1 : suggestionScroll + 1,
            (int)suggestions.size());
        DrawText(more.c_str(), (int)(b.x + b.width) - MeasureText(more.c_str(), 16) - 6,
            (int)(b.y + b.height) - 18, 16, GRAY);
    }
}
