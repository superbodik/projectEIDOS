#include "FoodSystem.h"
#include "../World/BlockType.h"

namespace Food {

    static const std::vector<Def> TABLE = {
        { (int)BlockType::Berries,       "Berries",        0.08f, 0.05f, Fruit,      0.16f, 0.0f,
          "Ripe bushes regrow. Juicy enough to matter when water is far." },
        { (int)BlockType::Acorn,         "Acorn",          0.06f, 0.00f, Grain,      0.14f, 0.0f,
          "Falls from oak leaves. Bitter raw, but it is starch." },
        { (int)BlockType::Grubs,         "Grubs",          0.07f, 0.01f, Protein,    0.18f, 0.0f,
          "Found under bark. The only protein before the hunting spear." },
        { (int)BlockType::BirdEgg,       "Bird Egg",       0.12f, 0.00f, Protein,    0.22f, 0.0f,
          "Rare find in the canopy. Worth the climb." },
        { (int)BlockType::BrownMushroom, "Brown Mushroom", 0.05f, 0.02f, Vegetables, 0.15f, 0.0f,
          "Safe. Grows in shade and on cave floors." },
        { (int)BlockType::RedMushroom,   "Red Mushroom",   0.00f, 0.00f, None,       0.00f, 0.12f,
          "Poisonous. The world teaches this one the hard way." }
    };

    const std::vector<Def>& All() { return TABLE; }

    const Def* Get(int id) {
        for (const Def& d : TABLE)
            if (d.id == id) return &d;
        return nullptr;
    }

    bool IsEdible(int id) { return Get(id) != nullptr; }

    const char* NutrientName(int nutrient) {
        switch (nutrient) {
        case Grain:      return "Grain";
        case Vegetables: return "Vegetables";
        case Fruit:      return "Fruit";
        case Protein:    return "Protein";
        default:          return "-";
        }
    }

}
