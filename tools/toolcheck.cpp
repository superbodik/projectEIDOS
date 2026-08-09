#include "../src/Inventory/MiningRules.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <cstdlib>

static const char* CatName(ToolCategory c) {
    switch (c) {
    case ToolCategory::None:    return "None";
    case ToolCategory::Pickaxe: return "Pickaxe";
    case ToolCategory::Axe:     return "Axe";
    case ToolCategory::Shovel:  return "Shovel";
    case ToolCategory::Hoe:     return "Hoe";
    case ToolCategory::Knife:   return "Knife";
    }
    return "?";
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    int fails = 0;

    printf("=== stone cannot be hand-mined, a pickaxe unlocks it ===\n");
    const BlockType ROCKS[] = {
        BlockType::Stone, BlockType::Granite, BlockType::Basalt, BlockType::Limestone,
        BlockType::Chalk, BlockType::Quartzite, BlockType::NativeCopper, BlockType::Galena,
        BlockType::Barite, BlockType::Wolframite, BlockType::Kimberlite
    };
    for (BlockType rock : ROCKS) {
        float byHand = MiningRules::TimeToBreak(rock, BlockType::Air);
        float byPick = MiningRules::TimeToBreak(rock, BlockType::StonePickaxe);
        float byAxe = MiningRules::TimeToBreak(rock, BlockType::StoneAxe);

        bool ok = (byHand < 0.0f) && (byPick > 0.0f) && (byAxe < 0.0f);
        printf("  %-16s hand=%.2f pick=%.2f axe=%.2f  %s\n",
            BlockInfo::GetName((int)rock).c_str(), byHand, byPick, byAxe,
            ok ? "ok" : "FAIL");
        if (!ok) fails++;
    }

    printf("\n=== wood is soft-gated: slow by hand, fast with an axe ===\n");
    const BlockType LOGS[] = {
        BlockType::OakLog, BlockType::SpruceLog, BlockType::BirchLog,
        BlockType::WillowLog, BlockType::FirLog
    };
    for (BlockType log : LOGS) {
        float byHand = MiningRules::TimeToBreak(log, BlockType::Air);
        float byAxe = MiningRules::TimeToBreak(log, BlockType::StoneAxe);
        float byPick = MiningRules::TimeToBreak(log, BlockType::StonePickaxe);

        bool ok = (byHand > 0.0f) && (byAxe > 0.0f) && (byHand > byAxe * 4.0f) && (byPick == byHand);
        printf("  %-16s hand=%.2f axe=%.2f  %s\n",
            BlockInfo::GetName((int)log).c_str(), byHand, byAxe, ok ? "ok" : "FAIL");
        if (!ok) fails++;
    }

    printf("\n=== loose ground: a shovel helps, hands are fine ===\n");
    const BlockType GROUND[] = {
        BlockType::Dirt, BlockType::Sand, BlockType::Gravel, BlockType::Clay, BlockType::Mud
    };
    for (BlockType g : GROUND) {
        float byHand = MiningRules::TimeToBreak(g, BlockType::Air);
        float byShovel = MiningRules::TimeToBreak(g, BlockType::StoneShovel);
        bool ok = (byHand > 0.0f) && (byShovel > 0.0f) && (byHand > byShovel);
        printf("  %-16s hand=%.2f shovel=%.2f  %s\n",
            BlockInfo::GetName((int)g).c_str(), byHand, byShovel, ok ? "ok" : "FAIL");
        if (!ok) fails++;
    }

    printf("\n=== every stone tool reports its own category correctly ===\n");
    struct ToolCheck { BlockType tool; ToolCategory expect; };
    const ToolCheck TOOLS[] = {
        { BlockType::StonePickaxe, ToolCategory::Pickaxe },
        { BlockType::StoneAxe,     ToolCategory::Axe },
        { BlockType::StoneShovel,  ToolCategory::Shovel },
        { BlockType::StoneHoe,     ToolCategory::Hoe },
        { BlockType::StoneKnife,   ToolCategory::Knife },
    };
    for (auto& t : TOOLS) {
        ToolCategory got = MiningRules::GetToolCategory(t.tool);
        ToolTier tier = MiningRules::GetToolTier(t.tool);
        bool ok = (got == t.expect) && (tier == ToolTier::Stone);
        printf("  %-16s -> %-8s tier=%d  %s\n",
            BlockInfo::GetName((int)t.tool).c_str(), CatName(got), (int)tier,
            ok ? "ok" : "FAIL");
        if (!ok) fails++;
    }

    printf("\n=== plants, pebbles, food stay instant like before ===\n");
    const BlockType INSTANT[] = {
        BlockType::TallGrass, BlockType::Rose, BlockType::StonePebble,
        BlockType::CopperPebble, BlockType::Torch, BlockType::BrownMushroom
    };
    for (BlockType b : INSTANT) {
        float t = MiningRules::TimeToBreak(b, BlockType::Air);
        bool ok = (t > 0.0f && t <= 0.2f);
        printf("  %-16s hand=%.2f  %s\n", BlockInfo::GetName((int)b).c_str(), t,
            ok ? "ok" : "FAIL");
        if (!ok) fails++;
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    fflush(stdout);
    std::_Exit(fails ? 1 : 0);
}
