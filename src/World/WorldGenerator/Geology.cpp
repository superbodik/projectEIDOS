#include "../WorldGenerator.h"
#include <algorithm>
#include <cmath>
#include <climits>

namespace {

struct Bed { int top; BlockType rock; };

const Bed BEDS_SHIELD[] = {
    { 92, BlockType::Quartzite }, { 68, BlockType::Gneiss }, { 44, BlockType::Schist },
    { 24, BlockType::Granite },   { 10, BlockType::Diorite }, { INT_MIN, BlockType::Gabbro }
};
const Bed BEDS_VOLCANIC[] = {
    { 98, BlockType::Rhyolite }, { 76, BlockType::Dacite }, { 52, BlockType::Andesite },
    { 28, BlockType::Basalt },   { 12, BlockType::Gabbro }, { INT_MIN, BlockType::Granite }
};
const Bed BEDS_BASIN[] = {
    { 110, BlockType::Claystone }, { 96, BlockType::Conglomerate }, { 74, BlockType::Sandstone },
    { 56, BlockType::Shale },      { 38, BlockType::Limestone },    { 22, BlockType::Dolomite },
    { 8,  BlockType::Chert },      { INT_MIN, BlockType::Granite }
};
const Bed BEDS_KARST[] = {
    { 100, BlockType::Chalk }, { 70, BlockType::Limestone }, { 48, BlockType::Dolomite },
    { 30, BlockType::Marble }, { 14, BlockType::Schist },    { INT_MIN, BlockType::Granite }
};
const Bed BEDS_BELT[] = {
    { 94, BlockType::Slate },     { 72, BlockType::Phyllite }, { 50, BlockType::Schist },
    { 30, BlockType::Quartzite }, { 14, BlockType::Gneiss },   { INT_MIN, BlockType::Granite }
};

struct Suite { const Bed* beds; int count; BlockType vein; const char* name; };

const Suite SUITES[] = {
    { BEDS_SHIELD,   6, BlockType::Quartzite, "Shield" },
    { BEDS_VOLCANIC, 6, BlockType::Rhyolite,  "Volcanic Province" },
    { BEDS_BASIN,    8, BlockType::Chert,     "Sedimentary Basin" },
    { BEDS_KARST,    6, BlockType::Marble,    "Carbonate Platform" },
    { BEDS_BELT,     6, BlockType::Quartzite, "Fold Belt" }
};

struct OreDef { BlockType ore; int yMin; int yMax; float rate; };

BlockType OrePebbleFor(BlockType ore) {
    switch (ore) {
    case BlockType::BituminousCoal:
    case BlockType::Lignite:        return BlockType::CoalPebble;
    case BlockType::Hematite:
    case BlockType::Magnetite:
    case BlockType::Limonite:       return BlockType::IronPebble;
    case BlockType::NativeCopper:
    case BlockType::Malachite:
    case BlockType::Tetrahedrite:   return BlockType::CopperPebble;
    case BlockType::Cassiterite:
    case BlockType::Bismuthinite:   return BlockType::TinPebble;
    case BlockType::Sphalerite:     return BlockType::ZincPebble;
    case BlockType::NativeGold:     return BlockType::GoldPebble;
    case BlockType::NativeSilver:   return BlockType::SilverPebble;
    case BlockType::Galena:         return BlockType::LeadPebble;
    case BlockType::Kimberlite:     return BlockType::DiamondPebble;
    case BlockType::Barite:         return BlockType::BaritePebble;
    case BlockType::Fluorite:       return BlockType::FluoritePebble;
    case BlockType::Phosphorite:    return BlockType::PhosphoritePebble;
    case BlockType::Sylvite:        return BlockType::PotashPebble;
    case BlockType::Wolframite:     return BlockType::TungstenPebble;
    case BlockType::Uraninite:      return BlockType::UraniumPebble;
    default:                         return BlockType::StonePebble;
    }
}

const OreDef ORE_CARBONATE[] = {
    { BlockType::Galena,         8, 58, 0.0060f },
    { BlockType::Sphalerite,    10, 62, 0.0060f },
    { BlockType::Limonite,      30, 78, 0.0045f },
    { BlockType::BituminousCoal, 18, 70, 0.0055f },
    { BlockType::Barite,        12, 64, 0.0045f },
    { BlockType::Fluorite,       6, 56, 0.0055f }
};
const OreDef ORE_SEDIMENTARY[] = {
    { BlockType::BituminousCoal, 14, 82, 0.0130f },
    { BlockType::Lignite,        52, 96, 0.0060f },
    { BlockType::Hematite,        8, 66, 0.0075f },
    { BlockType::Limonite,       34, 84, 0.0050f },
    { BlockType::Phosphorite,    40, 90, 0.0035f },
    { BlockType::Sylvite,        58, 98, 0.0075f }
};
const OreDef ORE_IGNEOUS[] = {
    { BlockType::Magnetite,       6, 62, 0.0080f },
    { BlockType::NativeCopper,   14, 70, 0.0070f },
    { BlockType::Malachite,      26, 74, 0.0055f },
    { BlockType::Bismuthinite,    4, 26, 0.0030f },
    { BlockType::Uraninite,       4, 40, 0.0028f }
};
const OreDef ORE_GRANITIC[] = {
    { BlockType::Cassiterite,     6, 54, 0.0075f },
    { BlockType::Tetrahedrite,   12, 60, 0.0050f },
    { BlockType::Bismuthinite,    4, 30, 0.0035f },
    { BlockType::NativeSilver,    5, 34, 0.0035f },
    { BlockType::Wolframite,      8, 52, 0.0055f }
};
const OreDef ORE_METAMORPHIC[] = {
    { BlockType::NativeGold,      5, 46, 0.0050f },
    { BlockType::NativeSilver,    8, 50, 0.0045f },
    { BlockType::Hematite,       10, 64, 0.0055f },
    { BlockType::Tetrahedrite,   14, 62, 0.0040f }
};

template <int N>
constexpr int CountOf(const OreDef(&)[N]) { return N; }

struct OreTable { const OreDef* defs; int count; };

OreTable HostOreTable(BlockType host) {
    if (WorldGenerator::IsCarbonate(host))
        return { ORE_CARBONATE, CountOf(ORE_CARBONATE) };
    if (WorldGenerator::IsSedimentary(host))
        return { ORE_SEDIMENTARY, CountOf(ORE_SEDIMENTARY) };
    if (host == BlockType::Granite || host == BlockType::Diorite ||
        host == BlockType::Rhyolite)
        return { ORE_GRANITIC, CountOf(ORE_GRANITIC) };
    if (WorldGenerator::IsIgneous(host))
        return { ORE_IGNEOUS, CountOf(ORE_IGNEOUS) };
    if (WorldGenerator::IsMetamorphic(host))
        return { ORE_METAMORPHIC, CountOf(ORE_METAMORPHIC) };
    return { ORE_SEDIMENTARY, CountOf(ORE_SEDIMENTARY) };
}

}

const char* WorldGenerator::RockSuiteName(RockSuite s) {
    int i = (int)s;
    if (i < 0 || i >= (int)RockSuite::Count) return "Unknown";
    return SUITES[i].name;
}

bool WorldGenerator::IsCarbonate(BlockType r) {
    return r == BlockType::Limestone || r == BlockType::Dolomite ||
        r == BlockType::Chalk || r == BlockType::Marble;
}

bool WorldGenerator::IsSedimentary(BlockType r) {
    return r == BlockType::Shale || r == BlockType::Sandstone ||
        r == BlockType::RedSandstone || r == BlockType::Claystone ||
        r == BlockType::Conglomerate || r == BlockType::Chert || IsCarbonate(r);
}

bool WorldGenerator::IsIgneous(BlockType r) {
    return r == BlockType::Granite || r == BlockType::Diorite || r == BlockType::Gabbro ||
        r == BlockType::Rhyolite || r == BlockType::Basalt ||
        r == BlockType::Andesite || r == BlockType::Dacite;
}

bool WorldGenerator::IsMetamorphic(BlockType r) {
    return r == BlockType::Quartzite || r == BlockType::Slate || r == BlockType::Phyllite ||
        r == BlockType::Schist || r == BlockType::Gneiss || r == BlockType::Marble;
}

BlockType WorldGenerator::BedRockAt(RockSuite suite, int y) {
    const Suite& s = SUITES[(int)suite];
    for (int i = 0; i < s.count; ++i)
        if (y >= s.beds[i].top) return s.beds[i].rock;
    return s.beds[s.count - 1].rock;
}

RockColumn WorldGenerator::ComputeRock(int x, int z) {
    RockColumn rc;

    float fx = (float)x, fz = (float)z;
    float wx = fx + noiseWarpX.GetNoise(fx, fz) * 140.0f;
    float wz = fz + noiseWarpZ.GetNoise(fx, fz) * 140.0f;

    float cell = noiseProvince.GetNoise(wx, wz) * 0.5f + 0.5f;
    int idx = (int)(cell * (float)RockSuite::Count);
    if (idx < 0) idx = 0;
    if (idx >= (int)RockSuite::Count) idx = (int)RockSuite::Count - 1;
    rc.suite = (RockSuite)idx;

    rc.bedOffset = noiseBed.GetNoise(fx, fz) * 13.0f +
        noiseBed.GetNoise(fx * 4.3f + 811.0f, fz * 4.3f - 233.0f) * 3.5f;

    float vein = noiseVein.GetNoise(fx, fz);
    if (vein > 0.68f) {
        float hv = Hash(x >> 3, z >> 3);
        rc.veinActive = true;
        rc.veinRock = SUITES[idx].vein;
        rc.veinBottom = 5 + (int)(hv * 12.0f);
        rc.veinTop = rc.veinBottom + 26 + (int)(Hash((x >> 3) + 61, (z >> 3) - 47) * 52.0f);
    }

    return rc;
}

RockColumn WorldGenerator::GetRockColumn(int x, int z) { return ComputeRock(x, z); }

BlockType WorldGenerator::GetSurfaceRock(int x, int z) {
    ColumnInfo col = GetColumnInfo(x, z);
    return GetStoneType(x, std::max(1, col.height - ROCK_PROBE_DEPTH), z, col);
}

BlockType WorldGenerator::GetStoneType(int x, int y, int z, const ColumnInfo& col) const {
    const RockColumn& rc = col.rock;

    if (rc.veinActive && y >= rc.veinBottom && y <= rc.veinTop)
        return rc.veinRock;

    int by = y + (int)rc.bedOffset;

    BlockType rock = BedRockAt(rc.suite, by);

    float mottle = Hash3(x >> 1, by >> 2, z >> 1);
    if (mottle > 0.955f) {
        BlockType neighbour = BedRockAt(rc.suite, by + ((mottle > 0.978f) ? 14 : -14));
        if (neighbour != rock) return neighbour;
    }

    bool nearSurface = (y > col.height - 14);

    if (nearSurface && (col.biome == BiomeType::Scorched ||
        col.biome == BiomeType::SaltFlat))
        return BlockType::RedSandstone;

    if (nearSurface && (col.biome == BiomeType::Desert ||
        col.biome == BiomeType::TemperateDesert ||
        col.biome == BiomeType::Shrubland) && IsSedimentary(rock))
        return BlockType::Sandstone;

    return rock;
}

BlockType WorldGenerator::GetOreBlock(int x, int y, int z, BlockType host,
    const RockColumn& rc) const {
    OreTable table = HostOreTable(host);
    const OreDef* defs = table.defs;

    bool inVein = rc.veinActive && y >= rc.veinBottom && y <= rc.veinTop;
    float boost = (inVein ? 3.2f : 1.0f) * std::clamp(gen.oreDensity, 0.05f, 6.0f);

    float h = Hash3(x, y, z);
    float acc = 0.0f;
    for (int i = 0; i < table.count; ++i) {
        if (y < defs[i].yMin || y > defs[i].yMax) continue;
        acc += defs[i].rate * boost;
        if (h > 1.0f - acc) return defs[i].ore;
    }

    if (rc.suite == RockSuite::Shield && y >= 1 && y <= 22 && h < 0.0012f)
        return BlockType::Kimberlite;

    return host;
}

BlockType WorldGenerator::PebbleForRock(BlockType rock) const {
    switch (rock) {
    case BlockType::Granite:
    case BlockType::Diorite:
    case BlockType::Gabbro:         return BlockType::GranitePebble;
    case BlockType::Basalt:
    case BlockType::Andesite:
    case BlockType::Rhyolite:
    case BlockType::Dacite:         return BlockType::BasaltPebble;
    case BlockType::Limestone:
    case BlockType::Dolomite:
    case BlockType::Chalk:
    case BlockType::Marble:         return BlockType::LimestonePebble;
    case BlockType::Sandstone:
    case BlockType::RedSandstone:
    case BlockType::Conglomerate:
    case BlockType::Claystone:      return BlockType::SandstonePebble;
    case BlockType::Chert:
    case BlockType::Shale:
    case BlockType::Slate:          return BlockType::FlintPebble;
    case BlockType::Gneiss:
    case BlockType::Schist:
    case BlockType::Phyllite:       return BlockType::GranitePebble;
    default:                         return BlockType::StonePebble;
    }
}

BlockType WorldGenerator::GetDecorationBlock(int x, int y, int z, const ColumnInfo& col) {
    const int        surfaceY = col.height;
    const BiomeType  biome = col.biome;
    const RiverInfo& river = col.river;

    if (y > surfaceY + 24 || surfaceY <= SEA_LEVEL) return BlockType::Air;

    if (river.channel) return BlockType::Air;
    if (river.active && y <= river.waterLevel) return BlockType::Air;

    if (CarvesAt(x, surfaceY, z, surfaceY, river)) return BlockType::Air;

    for (int dx = -3; dx <= 3; dx++) {
        for (int dz = -3; dz <= 3; dz++) {
            int tx = x + dx, tz = z + dz;
            float chance = Hash(tx, tz);
            if (chance <= 0.978f) continue;
            float sizeRng = Hash(tx, tz + 7);

            bool isTree = false, isConical = false;
            BlockType logT = BlockType::OakLog, leavesT = BlockType::OakLeaves;
            int trunkH = 4 + (int)(sizeRng * 2.0f);

            BiomeType tBiome = GetBiome(tx, tz);
            switch (tBiome) {
            case BiomeType::TemperateDeciduousForest:
            case BiomeType::TemperateRainforest:
                if (chance > 0.993f) { isTree = true; trunkH = 5 + (int)(sizeRng * 3); }
                break;
            case BiomeType::Taiga:
                if (chance > 0.988f) {
                    isTree = true; isConical = true;
                    logT = BlockType::SpruceLog; leavesT = BlockType::SpruceLeaves;
                    trunkH = 7 + (int)(sizeRng * 5);
                }
                break;
            case BiomeType::TropicalRainforest:
                if (chance > 0.980f) {
                    isTree = true;
                    logT = BlockType::JungleLog; leavesT = BlockType::JungleLeaves;
                    trunkH = 8 + (int)(sizeRng * 6);
                }
                break;
            case BiomeType::Savanna:
                if (chance > 0.996f) {
                    isTree = true;
                    logT = BlockType::AcaciaLog; leavesT = BlockType::AcaciaLeaves;
                    trunkH = 4 + (int)(sizeRng * 2);
                }
                break;
            case BiomeType::Tundra:
                if (chance > 0.998f) {
                    isTree = true; isConical = true;
                    logT = BlockType::SpruceLog; leavesT = BlockType::SpruceLeaves;
                    trunkH = 3 + (int)(sizeRng * 2);
                }
                break;
            default: break;
            }

            if (!isTree) continue;

            float tb = GetBaseHeight(tx, tz);
            RiverInfo tr = ComputeRiver(tx, tz, tb);
            int baseY = (int)tr.terrainH;
            if (tr.channel || baseY <= SEA_LEVEL) continue;
            if (IsCaveAt(tx, baseY, tz, baseY)) continue;

            int treeTop = baseY + trunkH;
            if (y <= baseY || y > treeTop) continue;

            if (dx == 0 && dz == 0 && y < treeTop) return logT;

            if (isConical) {
                int layer = treeTop - y;
                if (layer >= 0 && layer <= trunkH - 2) {
                    int r = (layer % 2 == 0) ? 1 : 2;
                    if (layer == 0) r = 1;
                    if (std::abs(dx) <= r && std::abs(dz) <= r) {
                        if (std::abs(dx) == r && std::abs(dz) == r && r > 1 && Hash(tx + y, tz - y) > 0.5f) return BlockType::Air;
                        if (!(dx == 0 && dz == 0 && y < treeTop)) return leavesT;
                    }
                }
            }
            else {
                if (y >= treeTop - 2 && y <= treeTop) {
                    int r = (y == treeTop) ? 1 : 2;
                    if (std::abs(dx) <= r && std::abs(dz) <= r) {
                        if (std::abs(dx) == r && std::abs(dz) == r) {
                            if (y == treeTop || Hash(tx + y, tz) > 0.4f) return BlockType::Air;
                        }

                        if (!(dx == 0 && dz == 0 && y < treeTop)) return leavesT;
                    }
                }
            }
        }
    }

    if (y != surfaceY + 1) return BlockType::Air;

    {
        BlockType ground = ResolveSurface(x, surfaceY, z, col);
        bool soil = (ground == BlockType::Grass || ground == BlockType::Dirt ||
            ground == BlockType::CoarseDirt || ground == BlockType::Mud ||
            ground == BlockType::Peat || ground == BlockType::Sand ||
            ground == BlockType::RedSand);
        if (!soil) return BlockType::Air;
    }

    float dcR = Hash(x + 91, z - 44);
    if (river.active && !river.channel && surfaceY <= river.waterLevel + 2 && dcR > 0.84f)
        return BlockType::Reed;
    if (biome == BiomeType::Beach && surfaceY <= SEA_LEVEL + 2 && dcR > 0.93f)
        return BlockType::Reed;

    float lushN = (noiseDetail.GetNoise((float)x * 2.3f, (float)z * 2.3f) + 1.0f) * 0.5f;
    float lush = lushN * lushN;
    float dcG = Hash(x, z + 500);
    float dcF = Hash(x + 300, z + 700);
    float dcS = Hash(x + 811, z + 271);
    float dcB = Hash(x - 617, z + 1279);

    bool berryBiome = (biome == BiomeType::Taiga ||
        biome == BiomeType::TemperateDeciduousForest ||
        biome == BiomeType::TemperateRainforest);
    if (berryBiome && surfaceY > SEA_LEVEL + 1) {
        float patch = (noiseDetail.GetNoise((float)x * 0.55f + 4400.0f,
            (float)z * 0.55f - 2100.0f) + 1.0f) * 0.5f;
        if (patch > 0.63f && dcB > 0.962f - (patch - 0.63f) * 0.55f) {
            BlockType ground = ResolveSurface(x, surfaceY, z, col);
            if (ground == BlockType::Grass || ground == BlockType::Dirt ||
                ground == BlockType::CoarseDirt)
                return (dcB > 0.986f) ? BlockType::BerryBushRipe : BlockType::BerryBush;
        }
    }

    switch (biome) {
    case BiomeType::Desert: case BiomeType::Scorched:
        if (dcG > 0.998f - lush * 0.006f) return BlockType::Cactus;
        if (dcF > 0.990f - lush * 0.012f) return BlockType::DeadBush;
        break;
    case BiomeType::Taiga:
        if (dcS > 0.992f - lush * 0.012f) return BlockType::BrownMushroom;
        if (dcG > 1.0f - 0.10f * (0.06f + lush)) return BlockType::TallGrass;
        break;
    case BiomeType::Swamp:
        if (dcG > 1.0f - 0.26f * (0.10f + lush)) return BlockType::TallGrass;
        if (dcS > 0.972f) return BlockType::BrownMushroom;
        if (dcF > 0.984f) return BlockType::Reed;
        break;
    case BiomeType::Bayou:
        if (surfaceY <= SEA_LEVEL + 2 && dcF > 0.955f) return BlockType::Cattail;
        if (dcG > 1.0f - 0.30f * (0.12f + lush)) return BlockType::Fern;
        if (dcS > 0.968f) return BlockType::Toadstool;
        break;
    case BiomeType::Floodplain:
        if (surfaceY <= SEA_LEVEL + 1 && dcF > 0.930f) return BlockType::LilyPad;
        if (dcG > 1.0f - 0.34f * (0.14f + lush)) return BlockType::TallGrass;
        if (dcS > 0.962f) return BlockType::Cattail;
        break;
    case BiomeType::Bog:
        if (dcG > 1.0f - 0.22f * (0.08f + lush)) return BlockType::CranberryBush;
        if (dcS > 0.966f) return BlockType::Toadstool;
        if (dcF > 0.980f) return BlockType::Reed;
        break;
    case BiomeType::ConiferousForest:
        if (dcG > 1.0f - 0.20f * (0.07f + lush)) return BlockType::TallGrass;
        if (dcS > 0.958f) return BlockType::Toadstool;
        if (lush > 0.40f && dcF > 0.986f) return BlockType::Dandelion;
        break;
    case BiomeType::Highland:
        if (dcG > 1.0f - 0.30f * (0.10f + lush)) return BlockType::TallGrass;
        if (dcF > 0.948f) return BlockType::Clover;
        break;
    case BiomeType::Beach:
        if (dcF > 0.972f) return BlockType::DuneGrass;
        break;
    case BiomeType::Mangrove:
        if (dcG > 1.0f - 0.20f * (0.08f + lush)) return BlockType::Reed;
        if (dcS > 0.986f) return BlockType::Fern;
        break;
    case BiomeType::BambooForest:
        if (dcG > 1.0f - 0.30f * (0.12f + lush)) return BlockType::Reed;
        if (dcF > 0.980f) return BlockType::Fern;
        break;
    case BiomeType::Moorland:
        if (dcG > 1.0f - 0.16f * (0.06f + lush)) return BlockType::TallGrass;
        if (dcS > 0.988f) return BlockType::BrownMushroom;
        break;
    case BiomeType::Steppe:
        if (dcG > 1.0f - 0.18f * (0.05f + lush)) return BlockType::TallGrass;
        if (dcF > 0.995f) return BlockType::Dandelion;
        break;
    case BiomeType::Shrubland:
        if (dcF > 0.976f - lush * 0.010f) return BlockType::DeadBush;
        if (dcG > 1.0f - 0.08f * (0.04f + lush)) return BlockType::TallGrass;
        break;
    case BiomeType::BirchForest:
        if (dcG > 1.0f - 0.22f * (0.06f + lush)) return BlockType::TallGrass;
        if (lush > 0.40f && dcF > 0.980f) return BlockType::Dandelion;
        if (dcS > 0.994f) return BlockType::RedMushroom;
        break;
    case BiomeType::RedwoodForest:
        if (dcG > 1.0f - 0.18f * (0.06f + lush)) return BlockType::Fern;
        if (dcS > 0.988f) return BlockType::BrownMushroom;
        break;
    case BiomeType::AlpineMeadow:
        if (dcG > 1.0f - 0.24f * (0.08f + lush)) return BlockType::TallGrass;
        if (dcF > 0.966f) return (dcF > 0.984f) ? BlockType::Rose : BlockType::Dandelion;
        break;
    case BiomeType::ColdDesert:
        if (dcF > 0.9975f) return BlockType::DeadBush;
        break;
    case BiomeType::VolcanicWastes:
    case BiomeType::SaltFlat:
    case BiomeType::Glacier:
        break;
    case BiomeType::Tundra:
        if (dcG > 1.0f - 0.06f * (0.05f + lush)) return BlockType::TallGrass;
        if (dcS > 0.997f) return BlockType::BrownMushroom;
        break;
    case BiomeType::SnowyTundra: case BiomeType::IceSpikes:
        if (dcF > 0.9985f) return BlockType::DeadBush;
        break;
    case BiomeType::Mountains:
        if (surfaceY < 130) {
            if (dcG > 1.0f - 0.14f * (0.08f + lush)) return BlockType::TallGrass;
            if (lush > 0.40f && dcF > 0.985f) return BlockType::Dandelion;
            if (dcS > 0.992f) return BlockType::BrownMushroom;
        }
        break;
    case BiomeType::TropicalRainforest:
        if (dcG > 1.0f - 0.22f * (0.10f + lush)) return BlockType::TallGrass;
        if (lush > 0.25f && dcS > 0.92f && dcS < 0.975f) return BlockType::Fern;
        if (lush > 0.35f && dcF > 0.975f) return (dcF > 0.988f) ? BlockType::Rose : BlockType::Dandelion;
        if (dcS > 0.990f) return BlockType::BrownMushroom;
        break;
    case BiomeType::TemperateRainforest:
        if (dcG > 1.0f - 0.22f * (0.06f + lush)) return BlockType::TallGrass;
        if (lush > 0.30f && dcS > 0.94f && dcS < 0.985f) return BlockType::Fern;
        if (lush > 0.45f && dcF > 0.980f) return BlockType::Rose;
        if (dcS > 0.996f) return BlockType::BrownMushroom;
        break;
    default:
        if (dcG > 1.0f - 0.22f * (0.05f + lush)) return BlockType::TallGrass;
        if (lush > 0.55f && dcS > 0.975f && dcS < 0.992f) return BlockType::Fern;
        if (lush > 0.45f && dcF > 0.978f - lush * 0.012f)
            return (dcF > 0.991f) ? BlockType::Dandelion : BlockType::Rose;
        if (dcS > 0.9975f) return BlockType::BrownMushroom;
        if (lush > 0.50f && dcF < 0.0015f) return BlockType::Pumpkin;
        break;
    }

    if (y == surfaceY + 1) {
        float pebChance = Hash(x ^ 0xABCD, z ^ 0x1234);
        bool isBeach = (biome == BiomeType::Beach || biome == BiomeType::Desert || biome == BiomeType::TemperateDesert);

        float threshold = isBeach ? 0.992f : 0.997f;
        if (biome == BiomeType::Mountains) threshold = 0.990f;
        if (biome == BiomeType::Tundra || biome == BiomeType::SnowyTundra) threshold = 0.996f;

        if (pebChance > threshold) {
            int hostY = std::max(1, surfaceY - ROCK_PROBE_DEPTH);
            BlockType host = GetStoneType(x, hostY, z, col);

            float oreHash = Hash3(x, surfaceY - 15, z);
            if (oreHash > 0.74f) {
                int oreY = std::clamp(surfaceY - 30 - (int)(Hash(x - 331, z + 733) * 26.0f),
                    4, std::max(5, surfaceY - 8));
                BlockType deep = GetStoneType(x, oreY, z, col);

                if (col.rock.suite == RockSuite::Shield && oreHash > 0.9975f)
                    return BlockType::DiamondPebble;

                OreTable table = HostOreTable(deep);
                const OreDef* defs = table.defs;
                float total = 0.0f;
                for (int i = 0; i < table.count; ++i)
                    if (oreY >= defs[i].yMin - 12 && oreY <= defs[i].yMax + 12)
                        total += defs[i].rate;

                if (total > 0.0f) {
                    float pick = Hash(x + 5501, z - 3307) * total;
                    float acc = 0.0f;
                    for (int i = 0; i < table.count; ++i) {
                        if (oreY < defs[i].yMin - 12 || oreY > defs[i].yMax + 12) continue;
                        acc += defs[i].rate;
                        if (pick <= acc) return OrePebbleFor(defs[i].ore);
                    }
                }
            }

            return PebbleForRock(host);
        }
    }

    return BlockType::Air;
}

