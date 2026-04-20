#include "BlockInfo.h"

namespace BlockInfo {

    std::string GetName(int id) {
        switch (id) {
        case 1: return "Water";
        case 2: return "Water Source";
        case 3: return "Lava";
        case 4: return "Lava Source";

        case 5: return "Bedrock";
        case 6: return "Grass Block";
        case 7: return "Dirt";
        case 8: return "Coarse Dirt";
        case 9: return "Mud";
        case 10: return "Clay";
        case 11: return "Sand";
        case 12: return "Red Sand";
        case 13: return "Gravel";
        case 14: return "Silt";
        case 15: return "Peat";

        case 16: return "Stone";
        case 17: return "Cobblestone";
        case 18: return "Glass";

        case 20: return "Limestone";
        case 21: return "Chalk";
        case 22: return "Shale";
        case 23: return "Claystone";
        case 24: return "Sandstone";
        case 25: return "Red Sandstone";
        case 26: return "Conglomerate";
        case 27: return "Dolomite";
        case 28: return "Chert";

        case 30: return "Granite";
        case 31: return "Diorite";
        case 32: return "Gabbro";
        case 35: return "Rhyolite";
        case 36: return "Basalt";
        case 37: return "Andesite";
        case 38: return "Dacite";

        case 40: return "Quartzite";
        case 41: return "Slate";
        case 42: return "Phyllite";
        case 43: return "Schist";
        case 44: return "Gneiss";
        case 45: return "Marble";

        case 50: return "Native Copper";
        case 51: return "Malachite";
        case 52: return "Tetrahedrite";
        case 53: return "Hematite";
        case 54: return "Magnetite";
        case 55: return "Limonite";
        case 56: return "Bituminous Coal";
        case 57: return "Lignite";
        case 60: return "Native Gold";
        case 61: return "Native Silver";
        case 62: return "Cassiterite";
        case 63: return "Sphalerite";
        case 64: return "Bismuthinite";
        case 65: return "Galena";
        case 66: return "Kimberlite";

        case 100: return "Oak Log";
        case 101: return "Oak Leaves";
        case 102: return "Spruce Log";
        case 103: return "Spruce Leaves";
        case 104: return "Birch Log";
        case 105: return "Birch Leaves";
        case 106: return "Acacia Log";
        case 107: return "Acacia Leaves";
        case 108: return "Jungle Log";
        case 109: return "Jungle Leaves";

        case 110: return "Cactus";
        case 111: return "Tall Grass";
        case 112: return "Dead Bush";
        case 113: return "Rose";
        case 114: return "Dandelion";
        case 115: return "Brown Mushroom";
        case 116: return "Red Mushroom";
        case 117: return "Sugar Cane";
        case 118: return "Pumpkin";
        case 119: return "Melon";

        case 120: return "Snow";
        case 121: return "Ice";
        case 122: return "Packed Ice";

        default: return "Block #" + std::to_string(id);
        }
    }
}