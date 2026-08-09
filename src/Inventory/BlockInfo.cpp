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

        case 180: return "Barite";
        case 181: return "Fluorite";
        case 182: return "Phosphorite";
        case 183: return "Sylvite";
        case 184: return "Wolframite";
        case 185: return "Uraninite";

        case 128: return "Berry Bush";
        case 129: return "Berry Bush (ripe)";
        case 150: return "Berries";
        case 151: return "Acorn";
        case 152: return "Grubs";
        case 153: return "Bird Egg";
        case 154: return "Plant Fibre";
        case 155: return "Copper Nugget";
        case 156: return "Tin Nugget";
        case 157: return "Silver Nugget";
        case 158: return "Gold Nugget";
        case 160: return "Clay Lump";
        case 161: return "Unfired Crucible";
        case 162: return "Crucible";
        case 163: return "Unfired Pickaxe Mould";
        case 164: return "Pickaxe Mould";
        case 165: return "Tongs";
        case 166: return "Fire Pit";
        case 167: return "Fire Pit (burning)";
        case 168: return "Fire Pit (embers)";
        case 169: return "Ash";
        case 170: return "Cattail";
        case 171: return "Lily Pad";
        case 172: return "Cranberry Bush";
        case 173: return "Willow Log";
        case 174: return "Willow Leaves";
        case 175: return "Fir Log";
        case 176: return "Fir Leaves";
        case 177: return "Toadstool";
        case 178: return "Clover";
        case 179: return "Dune Grass";

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
        case 123: return "Oak Sapling";
        case 124: return "Oak Planks";
        case 125: return "Fern";
        case 126: return "Reed";
        case 127: return "Torch";

        case 130: return "Stone Pebble";
        case 131: return "Copper Ore Pebble";
        case 132: return "Iron Ore Pebble";
        case 133: return "Coal Ore Pebble";
        case 134: return "Gold Ore Pebble";
        case 135: return "Diamond Ore Pebble";
        case 136: return "Flint";
        case 137: return "Granite Pebble";
        case 138: return "Basalt Pebble";
        case 139: return "Limestone Pebble";
        case 140: return "Sandstone Pebble";
        case 141: return "Tin Ore Pebble";
        case 142: return "Silver Ore Pebble";
        case 143: return "Zinc Ore Pebble";

        case 190: return "Lead Ore Pebble";
        case 191: return "Barite Pebble";
        case 192: return "Fluorite Pebble";
        case 193: return "Phosphorite Nodule";
        case 194: return "Potash Salt Pebble";
        case 195: return "Tungsten Ore Pebble";
        case 196: return "Uranium Ore Pebble";

        case 200: return "Knapped Pick Head";
        case 201: return "Knapped Axe Head";
        case 202: return "Knapped Shovel Head";
        case 203: return "Knapped Hoe Head";
        case 204: return "Knapped Knife Blade";

        case 205: return "Stone Pickaxe";
        case 206: return "Stone Axe";
        case 207: return "Stone Shovel";
        case 208: return "Stone Hoe";
        case 209: return "Stone Knife";

        case 210: return "Stick";
        case 211: return "Plant Rope";

        default: return "Block #" + std::to_string(id);
        }
    }
}
