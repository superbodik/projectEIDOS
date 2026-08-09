#include "EidosEngine.h"
#include "../World/Chunk.h"
#include "../Inventory/FoodSystem.h"
#include "../Inventory/BlockInfo.h"
#include <algorithm>
#include <cmath>

bool EidosEngine::TryShapeClay() {
    if (player.currentMode == GameMode::Creative) return false;
    if (player.inventory.GetSelectedBlockID() != (int)BlockType::ClayLump) return false;

    bool wantMould = IsKeyDown(KEY_LEFT_CONTROL);
    int cost = wantMould ? 3 : 4;
    BlockType result = wantMould ? BlockType::UnfiredPickMould
        : BlockType::UnfiredCrucible;

    int have = player.inventory.CountItem((int)BlockType::ClayLump);
    if (have < cost) {
        debugSystem.Log("[POTTERY] Need " + std::to_string(cost) + " clay lumps, have " +
            std::to_string(have) + ". Hold CTRL to shape a mould instead.");
        return true;
    }

    player.inventory.RemoveItem((int)BlockType::ClayLump, cost);
    player.inventory.AddItem((int)result, 1);
    debugSystem.Log(std::string("[POTTERY] Shaped an ") + BlockInfo::GetName((int)result) +
        ". Wet clay cracks in fire - it has to be pit-fired first.");
    return true;
}

bool EidosEngine::TryMakePlanks() {
    if (player.currentMode == GameMode::Creative) return false;

    int selected = player.inventory.GetSelectedBlockID();
    bool isLog = selected == (int)BlockType::OakLog || selected == (int)BlockType::SpruceLog ||
        selected == (int)BlockType::BirchLog || selected == (int)BlockType::AcaciaLog ||
        selected == (int)BlockType::JungleLog || selected == (int)BlockType::WillowLog ||
        selected == (int)BlockType::FirLog;
    if (!isLog) return false;

    player.inventory.RemoveItem(selected, 1);
    player.inventory.AddItem((int)BlockType::OakPlanks, 4);
    debugSystem.Log("[CRAFT] Split a log into 4 planks.");
    return true;
}

bool EidosEngine::TryMakeSticks() {
    if (player.currentMode == GameMode::Creative) return false;
    if (player.inventory.GetSelectedBlockID() != (int)BlockType::OakPlanks) return false;

    player.inventory.RemoveItem((int)BlockType::OakPlanks, 1);
    player.inventory.AddItem((int)BlockType::Stick, 4);
    debugSystem.Log("[CRAFT] Split a plank into 4 sticks.");
    return true;
}

bool EidosEngine::TryMakeRope() {
    if (player.currentMode == GameMode::Creative) return false;
    if (player.inventory.GetSelectedBlockID() != (int)BlockType::PlantFibre) return false;

    const int cost = 4;
    int have = player.inventory.CountItem((int)BlockType::PlantFibre);
    if (have < cost) {
        debugSystem.Log("[CRAFT] Need " + std::to_string(cost) + " plant fibre for rope, have " +
            std::to_string(have) + ".");
        return true;
    }

    player.inventory.RemoveItem((int)BlockType::PlantFibre, cost);
    player.inventory.AddItem((int)BlockType::PlantRope, 1);
    debugSystem.Log("[CRAFT] Twisted 4 plant fibre into rope.");
    return true;
}

bool EidosEngine::TryKnap() {
    if (player.currentMode == GameMode::Creative) return false;

    int selected = player.inventory.GetSelectedBlockID();
    bool knappable = selected == (int)BlockType::StonePebble ||
        selected == (int)BlockType::FlintPebble ||
        selected == (int)BlockType::GranitePebble ||
        selected == (int)BlockType::BasaltPebble;
    if (!knappable) return false;

    BlockType head = BlockType::StonePickHead;
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_LEFT_SHIFT)) head = BlockType::StoneHoeHead;
    else if (IsKeyDown(KEY_LEFT_CONTROL)) head = BlockType::StoneAxeHead;
    else if (IsKeyDown(KEY_LEFT_ALT)) head = BlockType::StoneShovelHead;
    else if (IsKeyDown(KEY_LEFT_SHIFT)) head = BlockType::StoneKnifeBlade;

    // Flint knaps cleaner than field stone - it costs one fewer piece.
    int cost = (selected == (int)BlockType::FlintPebble) ? 2 : 3;
    int have = player.inventory.CountItem(selected);
    if (have < cost) {
        debugSystem.Log("[KNAP] Need " + std::to_string(cost) + " more of that pebble, have " +
            std::to_string(have) + ". CTRL=axe, ALT=shovel, SHIFT=knife, CTRL+SHIFT=hoe.");
        return true;
    }

    player.inventory.RemoveItem(selected, cost);
    player.inventory.AddItem((int)head, 1);
    debugSystem.Log(std::string("[KNAP] Knapped a ") + BlockInfo::GetName((int)head) + ".");
    return true;
}

bool EidosEngine::TryAssembleTool() {
    if (player.currentMode == GameMode::Creative) return false;

    struct Recipe { BlockType head; BlockType tool; };
    static const Recipe RECIPES[] = {
        { BlockType::StonePickHead,   BlockType::StonePickaxe },
        { BlockType::StoneAxeHead,    BlockType::StoneAxe },
        { BlockType::StoneShovelHead, BlockType::StoneShovel },
        { BlockType::StoneHoeHead,    BlockType::StoneHoe },
        { BlockType::StoneKnifeBlade, BlockType::StoneKnife },
    };

    int selected = player.inventory.GetSelectedBlockID();
    const Recipe* recipe = nullptr;
    for (const Recipe& r : RECIPES) {
        if ((int)r.head == selected) { recipe = &r; break; }
    }
    if (!recipe) return false;

    bool haveStick = player.inventory.CountItem((int)BlockType::Stick) >= 1;
    bool haveRope = player.inventory.CountItem((int)BlockType::PlantRope) >= 1;
    if (!haveStick || !haveRope) {
        debugSystem.Log(std::string("[CRAFT] A ") + BlockInfo::GetName(selected) +
            " needs a stick and rope to become a tool.");
        return true;
    }

    player.inventory.RemoveItem(selected, 1);
    player.inventory.RemoveItem((int)BlockType::Stick, 1);
    player.inventory.RemoveItem((int)BlockType::PlantRope, 1);
    player.inventory.AddItem((int)recipe->tool, 1);
    debugSystem.Log(std::string("[CRAFT] Bound a ") + BlockInfo::GetName((int)recipe->tool) +
        " together.");
    return true;
}

bool EidosEngine::TryEatHeldItem() {
    if (player.currentMode == GameMode::Creative) return false;

    int id = player.inventory.GetSelectedBlockID();
    const Food::Def* d = Food::Get(id);
    if (!d) return false;

    if (d->poison <= 0.0f && player.satiety > 0.97f && player.hydration > 0.97f) {
        debugSystem.Log("[SURVIVAL] Too full to eat that.");
        return true;
    }

    player.inventory.ConsumeSelectedItem();

    player.satiety = std::clamp(player.satiety + d->satiety, 0.0f, 1.0f);
    player.hydration = std::clamp(player.hydration + d->hydration, 0.0f, 1.0f);

    if (d->nutrient >= 0 && d->nutrient < Player::NUTRIENT_COUNT)
        player.nutrients[d->nutrient] =
        std::clamp(player.nutrients[d->nutrient] + d->amount, 0.0f, 1.0f);

    if (d->poison > 0.0f) {
        player.health = std::clamp(player.health - d->poison, 0.0f, 1.0f);
        player.satiety = std::clamp(player.satiety - 0.05f, 0.0f, 1.0f);
        debugSystem.Log(std::string("[SURVIVAL] ") + d->name + " was a mistake.");
    }
    else {
        debugSystem.Log(std::string("[SURVIVAL] Ate ") + d->name + " (+" +
            std::to_string((int)(d->amount * 100.0f)) + "% " +
            Food::NutrientName(d->nutrient) + ")");
    }

    return true;
}

void EidosEngine::GrantForage(int brokenId, int x, int y, int z) {
    if (player.currentMode != GameMode::Survival) return;

    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + z * 1274126177);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    float roll = (float)(h & 0xFFFFFF) / (float)0xFFFFFF;
    float roll2 = (float)((h >> 8) & 0xFFFFFF) / (float)0xFFFFFF;

    BlockType b = (BlockType)brokenId;

    bool isLeaves = (b == BlockType::OakLeaves || b == BlockType::SpruceLeaves ||
        b == BlockType::BirchLeaves || b == BlockType::AcaciaLeaves ||
        b == BlockType::JungleLeaves || b == BlockType::WillowLeaves ||
        b == BlockType::FirLeaves);
    if (isLeaves) {
        if (roll < 0.16f) {
            player.inventory.AddItem((int)BlockType::Stick, 1);
            debugSystem.Log("[FORAGE] A stick falls out of the leaves.");
        }
        if (b == BlockType::OakLeaves) {
            if (roll2 < 0.10f) {
                player.inventory.AddItem((int)BlockType::Acorn, 1);
                debugSystem.Log("[FORAGE] An acorn falls out.");
            }
            else if (roll2 < 0.115f) {
                player.inventory.AddItem((int)BlockType::BirdEgg, 1);
                debugSystem.Log("[FORAGE] A nest! One egg survived.");
            }
        }
        return;
    }

    if (b == BlockType::OakLog || b == BlockType::BirchLog ||
        b == BlockType::SpruceLog || b == BlockType::JungleLog ||
        b == BlockType::AcaciaLog) {
        if (roll < 0.14f) {
            player.inventory.AddItem((int)BlockType::Grubs, 1);
            debugSystem.Log("[FORAGE] Grubs under the bark.");
        }
        return;
    }

    if (b == BlockType::TallGrass || b == BlockType::Fern) {
        if (roll < 0.45f) player.inventory.AddItem((int)BlockType::PlantFibre, 1);
        return;
    }

    if (b == BlockType::Clay) {
        int n = 3 + (int)(roll * 2.4f);
        player.inventory.AddItem((int)BlockType::ClayLump, n);
        debugSystem.Log("[FORAGE] " + std::to_string(n) +
            "x Clay Lump - shape it while it is still wet.");
        return;
    }

    BlockType nugget = BlockType::Air;
    switch (b) {
    case BlockType::CopperPebble: nugget = BlockType::CopperNugget; break;
    case BlockType::TinPebble:    nugget = BlockType::TinNugget;    break;
    case BlockType::SilverPebble: nugget = BlockType::SilverNugget; break;
    case BlockType::GoldPebble:   nugget = BlockType::GoldNugget;   break;
    default: break;
    }

    if (nugget != BlockType::Air) {
        int n = 1 + (int)(roll * 2.2f);
        player.inventory.AddItem((int)nugget, n);
        debugSystem.Log("[FORAGE] " + std::to_string(n) + "x " +
            BlockInfo::GetName((int)nugget) + " — soft metal, it can only be cast.");
        return;
    }

    if (b == BlockType::BerryBushRipe) {
        int n = 1 + (int)(roll2 * 3.0f);
        player.inventory.AddItem((int)BlockType::Berries, n);
        debugSystem.Log("[FORAGE] Picked " + std::to_string(n) + " berries.");
        return;
    }
}

void EidosEngine::UpdateSurvival(float dt) {
    if (currentState != GameState::Playing) return;
    if (player.currentMode != GameMode::Survival) {
        player.health = 1.0f;
        return;
    }

    survivalTimer += dt;
    if (survivalTimer < 0.5f) return;
    float step = survivalTimer;
    survivalTimer = 0.0f;

    const float DAY = std::max(60.0f, rules.dayLength);

    float exertion = 1.0f;
    if (player.isSprinting) exertion = 2.2f;
    else if (!player.isGrounded) exertion = 1.35f;

    float tempStress = 0.0f;
    if (player.bodyTempC < 35.4f) tempStress = (35.4f - player.bodyTempC) * 0.35f;
    else if (player.bodyTempC > 38.0f) tempStress = (player.bodyTempC - 38.0f) * 0.45f;

    float hungerRate = (1.15f / DAY) * rules.hungerRate * exertion
        * (1.0f + tempStress * 0.5f);
    player.satiety = std::clamp(player.satiety - hungerRate * step, 0.0f, 1.0f);

    float thirstRate = (1.85f / DAY) * rules.thirstRate * exertion;
    if (player.bodyTempC > 37.5f) thirstRate *= 1.9f;
    player.hydration = std::clamp(player.hydration - thirstRate * step, 0.0f, 1.0f);

    float decay = (0.70f / DAY) * rules.nutrientDecay * step;
    for (int i = 0; i < Player::NUTRIENT_COUNT; i++)
        player.nutrients[i] = std::clamp(player.nutrients[i] - decay, 0.0f, 1.0f);

    float damage = 0.0f;
    if (player.satiety <= 0.0f)   damage += 0.014f;
    if (player.hydration <= 0.0f) damage += 0.028f;
    if (player.bodyTempC < rules.coldThreshold)
        damage += (rules.coldThreshold - player.bodyTempC) * 0.009f;
    if (player.bodyTempC > rules.heatThreshold)
        damage += (player.bodyTempC - rules.heatThreshold) * 0.011f;
    damage *= rules.damageScale;

    float worstNutrient = 1.0f;
    for (int i = 0; i < Player::NUTRIENT_COUNT; i++)
        worstNutrient = std::min(worstNutrient, player.nutrients[i]);

    float healthCeiling = std::min(1.0f, player.maxHealth);

    if (damage > 0.0f) {
        player.health = std::clamp(player.health - damage * step, 0.0f, healthCeiling);
        player.regenPool = 0.0f;
    }
    else if (player.satiety > rules.regenSatietyGate &&
        player.hydration > rules.regenThirstGate &&
        player.health < healthCeiling) {
        float cap = std::min(healthCeiling,
            rules.regenFloor + worstNutrient * (1.0f - rules.regenFloor));
        if (player.health < cap) {
            player.regenPool += 0.010f * step;
            if (player.regenPool >= 0.01f) {
                player.health = std::clamp(player.health + player.regenPool, 0.0f, cap);
                player.regenPool = 0.0f;
                player.satiety = std::clamp(player.satiety - 0.010f, 0.0f, 1.0f);
            }
        }
    }

    if (player.maxHealth < 1.0f) {
        player.maxHealth = std::min(1.0f, player.maxHealth + (1.0f / DAY) * step);
    }

    if (player.health <= 0.0f) {
        if (rules.oneLife) {
            debugSystem.Log("[SURVIVAL] ONE LIFE: this world is over.");
            worldDead = true;
        }

        if (rules.deathPenalty >= 1) {
            int dropped = 0;
            for (int i = 0; i < Inventory::INV_SIZE; i++) {
                if (player.inventory.slots[i].id == 0) continue;
                dropped += player.inventory.slots[i].count;
                player.inventory.slots[i] = { 0, 0 };
            }
            if (dropped > 0)
                debugSystem.Log("[SURVIVAL] You dropped " + std::to_string(dropped) +
                    " items where you fell.");
        }

        if (rules.deathPenalty >= 2) {
            player.maxHealth = std::max(0.35f, player.maxHealth - 0.20f);
            debugSystem.Log("[SURVIVAL] Max health down to " +
                std::to_string((int)(player.maxHealth * 100.0f)) + "% for a day.");
        }

        debugSystem.Log("[SURVIVAL] You died. Respawning.");
        player.health = std::min(0.5f, player.maxHealth);
        player.satiety = 0.35f;
        player.hydration = 0.45f;
        player.bodyTempC = 36.6f;

        int sx = (int)player.position.x;
        int sz = (int)player.position.z;
        int sy = worldGen.GetHeight(sx, sz) + 2;
        player.position = { (float)sx + 0.5f, (float)sy, (float)sz + 0.5f };
        player.velocity = { 0, 0, 0 };
    }

    int px = (int)floorf(player.position.x);
    int py = (int)floorf(player.position.y);
    int pz = (int)floorf(player.position.z);
    if (Chunk::IsWaterBlock(GetBlockAt(px, py, pz)) ||
        Chunk::IsWaterBlock(GetBlockAt(px, py + 1, pz))) {
        player.hydration = std::clamp(player.hydration + 0.30f * step, 0.0f, 1.0f);
    }
}
