#include "MiningRules.h"

namespace MiningRules {

ToolCategory GetToolCategory(BlockType heldItem) {
    switch (heldItem) {
    case BlockType::StonePickaxe: return ToolCategory::Pickaxe;
    case BlockType::StoneAxe:     return ToolCategory::Axe;
    case BlockType::StoneShovel:  return ToolCategory::Shovel;
    case BlockType::StoneHoe:     return ToolCategory::Hoe;
    case BlockType::StoneKnife:   return ToolCategory::Knife;
    default:                       return ToolCategory::None;
    }
}

ToolTier GetToolTier(BlockType heldItem) {
    switch (heldItem) {
    case BlockType::StonePickaxe:
    case BlockType::StoneAxe:
    case BlockType::StoneShovel:
    case BlockType::StoneHoe:
    case BlockType::StoneKnife:
        return ToolTier::Stone;
    default:
        return ToolTier::Hand;
    }
}

static MiningInfo Pick(float hardness) {
    return { ToolCategory::Pickaxe, hardness, true, 1.0f };
}
static MiningInfo Axe(float hardness) {
    return { ToolCategory::Axe, hardness, false, 8.0f };
}
static MiningInfo Shovel(float hardness) {
    return { ToolCategory::Shovel, hardness, false, 3.0f };
}
static MiningInfo Hand(float hardness = 0.15f) {
    return { ToolCategory::None, hardness, false, 1.0f };
}

MiningInfo GetMiningInfo(BlockType block) {
    switch (block) {
    // --- soft sedimentary: chalk and clay barely resist a pick ---
    case BlockType::Chalk:       return Pick(0.6f);
    case BlockType::Claystone:
    case BlockType::Shale:       return Pick(0.8f);

    // --- medium sedimentary ---
    case BlockType::Limestone:
    case BlockType::Dolomite:
    case BlockType::Marble:      return Pick(1.2f);
    case BlockType::Sandstone:
    case BlockType::RedSandstone:
    case BlockType::Conglomerate: return Pick(1.4f);

    // --- foliated / low-grade metamorphic ---
    case BlockType::Slate:
    case BlockType::Phyllite:    return Pick(1.6f);
    case BlockType::Schist:
    case BlockType::Gneiss:      return Pick(2.0f);
    case BlockType::Chert:       return Pick(2.2f);

    // --- igneous: coarse and fine grained alike, uniformly tough ---
    case BlockType::Granite:
    case BlockType::Diorite:
    case BlockType::Gabbro:
    case BlockType::Rhyolite:
    case BlockType::Basalt:
    case BlockType::Andesite:
    case BlockType::Dacite:      return Pick(2.5f);

    // --- recrystallised quartz: the hardest common rock ---
    case BlockType::Quartzite:   return Pick(3.0f);

    case BlockType::Stone:
    case BlockType::Cobblestone: return Pick(1.5f);
    case BlockType::Glass:       return Pick(0.3f);
    case BlockType::Bedrock:     return { ToolCategory::Pickaxe, 0.0f, true, 1.0f }; // unbreakable in practice

    // --- ore veins run a little tougher than their host average ---
    case BlockType::NativeCopper:
    case BlockType::Malachite:
    case BlockType::Tetrahedrite:
    case BlockType::Hematite:
    case BlockType::Magnetite:
    case BlockType::Limonite:
    case BlockType::BituminousCoal:
    case BlockType::Lignite:
    case BlockType::NativeGold:
    case BlockType::NativeSilver:
    case BlockType::Cassiterite:
    case BlockType::Sphalerite:
    case BlockType::Bismuthinite:
    case BlockType::Galena:
    case BlockType::Barite:
    case BlockType::Fluorite:
    case BlockType::Phosphorite:
    case BlockType::Sylvite:
    case BlockType::Wolframite:
    case BlockType::Uraninite:   return Pick(2.6f);
    case BlockType::Kimberlite:  return Pick(3.5f);

    // --- wood: an axe is the tool, but bare hands can still get there ---
    case BlockType::OakLog: case BlockType::SpruceLog: case BlockType::BirchLog:
    case BlockType::AcaciaLog: case BlockType::JungleLog:
    case BlockType::WillowLog: case BlockType::FirLog:
        return Axe(2.0f);
    case BlockType::OakPlanks:   return Axe(1.2f);

    // --- loose ground: a shovel is faster, hands work fine ---
    case BlockType::Dirt: case BlockType::CoarseDirt: case BlockType::Mud:
    case BlockType::Sand: case BlockType::RedSand: case BlockType::Gravel:
    case BlockType::Silt: case BlockType::Peat: case BlockType::Clay:
    case BlockType::Snow:
        return Shovel(0.6f);

    default:
        return Hand();
    }
}

float TimeToBreak(BlockType block, BlockType heldItem) {
    MiningInfo info = GetMiningInfo(block);
    if (info.hardness <= 0.0f) return -1.0f; // unbreakable

    ToolCategory held = GetToolCategory(heldItem);
    bool matches = (info.tool == ToolCategory::None) || (held == info.tool);

    if (info.hardGate && !matches) return -1.0f;

    float penalty = matches ? 1.0f : info.handPenalty;
    return info.hardness * penalty;
}

bool CanBreak(BlockType block, BlockType heldItem) {
    return TimeToBreak(block, heldItem) >= 0.0f;
}

}
