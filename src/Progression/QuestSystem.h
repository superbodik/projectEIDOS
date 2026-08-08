#pragma once
#include <string>
#include <vector>

class EidosEngine;

enum class QuestKind { Collect, CollectAny, CollectDistinct, ReachBiome, ReachDepth, ReachHeight };

struct Quest {
    std::string id;
    std::string title;
    std::string desc;
    QuestKind kind = QuestKind::Collect;
    int blockId = 0;
    int amount = 1;
    std::string biome;
    std::vector<int> anyOf;
};

struct Era {
    std::string name;
    std::string version;
    bool playable = false;
    std::vector<Quest> quests;
};

class QuestSystem {
public:
    QuestSystem();

    void Toggle();
    bool IsOpen() const { return isOpen; }

    void Update(EidosEngine& eng, float dt);
    void Draw(EidosEngine& eng, int sw, int sh);

    void Load(const std::string& worldPath);
    void Save(const std::string& worldPath) const;
    void Reset();

    int ActiveIndex() const;
    int CompletedCount() const;

private:
    bool isOpen = false;
    std::vector<Era> eras;
    std::vector<bool> done;
    int selected = 0;
    int scroll = 0;
    float checkTimer = 0.0f;
    float toastTimer = 0.0f;
    std::string toastText;

    void BuildQuests();
    bool CheckQuest(EidosEngine& eng, const Quest& q, int& progress, int& needed);
    const std::vector<Quest>& MainQuests() const { return eras[0].quests; }
};
