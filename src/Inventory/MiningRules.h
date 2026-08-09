#pragma once
#include "../World/BlockType.h"

enum class ToolCategory { None, Pickaxe, Axe, Shovel, Hoe, Knife };
enum class ToolTier { Hand = 0, Stone = 1 };

struct MiningInfo {
    ToolCategory tool = ToolCategory::None;
    float hardness = 0.15f;
    bool hardGate = false;
    float handPenalty = 1.0f;
};

namespace MiningRules {
    MiningInfo GetMiningInfo(BlockType block);
    ToolCategory GetToolCategory(BlockType heldItem);
    ToolTier GetToolTier(BlockType heldItem);

    float TimeToBreak(BlockType block, BlockType heldItem);
    bool CanBreak(BlockType block, BlockType heldItem);
}
