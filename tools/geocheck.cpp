#include "../src/World/WorldGenerator.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

static bool IsRock(BlockType b) {
    int id = (int)b;
    return (id >= 16 && id <= 45) && id != 17 && id != 18 && id != 19;
}

static bool IsOre(BlockType b) {
    int id = (int)b;
    return id >= 50 && id <= 66;
}

static bool IsPebble(BlockType b) {
    int id = (int)b;
    return id >= 130 && id <= 143;
}

int main() {
    WorldGenerator gen(1337);
    int fails = 0;

    printf("=== provinces ===\n");
    std::map<int, int> suiteHits;
    const int SPAN = 6000, STEP = 60;
    for (int x = -SPAN; x <= SPAN; x += STEP)
        for (int z = -SPAN; z <= SPAN; z += STEP)
            suiteHits[(int)gen.GetRockColumn(x, z).suite]++;

    int totalCols = 0;
    for (auto& kv : suiteHits) totalCols += kv.second;
    for (int s = 0; s < (int)RockSuite::Count; ++s) {
        double pct = 100.0 * suiteHits[s] / totalCols;
        printf("  %-20s %5.1f%%\n", WorldGenerator::RockSuiteName((RockSuite)s), pct);
        if (pct < 8.0) {
            printf("    FAIL suite too rare (want >= 8%%)\n");
            fails++;
        }
    }

    printf("=== province size ===\n");
    {
        int flips = 0, samples = 0;
        RockSuite prev = gen.GetRockColumn(-4000, 0).suite;
        for (int x = -4000; x <= 4000; x += 8) {
            RockSuite s = gen.GetRockColumn(x, 0).suite;
            if (s != prev) flips++;
            prev = s;
            samples++;
        }
        double avgRun = 8000.0 / std::max(1, flips);
        printf("  %d boundaries over 8000 blocks, mean province %.0f blocks\n", flips, avgRun);
        if (avgRun < 150.0) {
            printf("    FAIL provinces too small - reads as noise, not geology\n");
            fails++;
        }
        if (avgRun > 3000.0) {
            printf("    FAIL provinces too large - world lacks variety\n");
            fails++;
        }
    }

    printf("=== rock coverage ===\n");
    std::map<int, long> rockHits;
    std::map<int, long> oreHits;
    std::map<int, std::map<int, long>> oreHost;

    long stoneBlocks = 0;
    for (int x = -2400; x <= 2400; x += 37) {
        for (int z = -2400; z <= 2400; z += 37) {
            ColumnInfo col = gen.GetColumnInfo(x, z);
            for (int y = 2; y < std::min(col.height, 140); y += 3) {
                BlockType b = gen.GetBlockFast(x, y, z, col);
                if (IsRock(b)) { rockHits[(int)b]++; stoneBlocks++; }
                else if (IsOre(b)) {
                    oreHits[(int)b]++;
                    stoneBlocks++;
                    oreHost[(int)b][(int)col.rock.suite]++;
                }
            }
        }
    }

    const int wantRocks[] = { 20,21,22,23,24,25,26,27,28,30,31,32,35,36,37,38,40,41,42,43,44,45 };
    int present = 0;
    for (int id : wantRocks) {
        long n = rockHits.count(id) ? rockHits[id] : 0;
        double pct = 100.0 * n / std::max(1L, stoneBlocks);
        printf("  %-16s %7ld  %5.2f%%%s\n", BlockInfo::GetName(id).c_str(), n, pct,
            n == 0 ? "   <-- NEVER GENERATED" : "");
        if (n > 0) present++;
        else fails++;
    }
    printf("  %d/%d rock types reachable\n", present, (int)(sizeof(wantRocks) / sizeof(*wantRocks)));

    long plainStone = rockHits.count(16) ? rockHits[16] : 0;
    printf("  plain Stone filler: %ld (%.2f%%)\n", plainStone,
        100.0 * plainStone / std::max(1L, stoneBlocks));

    printf("=== ores ===\n");
    const int wantOres[] = { 50,51,52,53,54,55,56,57,60,61,62,63,64,65,66 };
    long oreTotal = 0;
    for (int id : wantOres) oreTotal += oreHits.count(id) ? oreHits[id] : 0;
    for (int id : wantOres) {
        long n = oreHits.count(id) ? oreHits[id] : 0;
        printf("  %-18s %6ld", BlockInfo::GetName(id).c_str(), n);
        if (n == 0) { printf("   <-- NEVER GENERATED"); fails++; }
        else {
            int bestSuite = -1; long bestN = 0;
            for (auto& kv : oreHost[id]) if (kv.second > bestN) { bestN = kv.second; bestSuite = kv.first; }
            printf("   mostly in %-20s (%.0f%%)",
                WorldGenerator::RockSuiteName((RockSuite)bestSuite),
                100.0 * bestN / n);
        }
        printf("\n");
    }
    double oreShare = 100.0 * oreTotal / std::max(1L, stoneBlocks);
    printf("  ore density %.2f%% of stone\n", oreShare);
    if (oreShare < 0.5 || oreShare > 8.0) {
        printf("    FAIL ore density out of range (want 0.5-8%%)\n");
        fails++;
    }

    printf("=== strata coherence ===\n");
    {
        double totalRuns = 0; int cols = 0;
        for (int x = -1500; x <= 1500; x += 97) {
            for (int z = -1500; z <= 1500; z += 97) {
                ColumnInfo col = gen.GetColumnInfo(x, z);
                if (col.height < 40) continue;
                BlockType prev = BlockType::Air;
                int runs = 0;
                for (int y = 2; y < col.height - 8; ++y) {
                    BlockType b = gen.GetBlockFast(x, y, z, col);
                    if (!IsRock(b)) continue;
                    if (b != prev) { runs++; prev = b; }
                }
                totalRuns += runs;
                cols++;
            }
        }
        double avg = totalRuns / std::max(1, cols);
        printf("  mean rock changes per column: %.1f\n", avg);
        if (avg > 22.0) {
            printf("    FAIL too speckled - bedrock is noise, not layered strata\n");
            fails++;
        }
        if (avg < 2.0) {
            printf("    FAIL no layering at all\n");
            fails++;
        }
    }

    printf("=== pebbles match bedrock ===\n");
    {
        long checked = 0, rockPeb = 0, orePeb = 0, mismatch = 0;
        std::map<int, long> pebHits;
        for (int x = -3000; x <= 3000; x += 13) {
            for (int z = -3000; z <= 3000; z += 13) {
                ColumnInfo col = gen.GetColumnInfo(x, z);
                int y = col.height + 1;
                BlockType b = gen.GetBlockFast(x, y, z, col);
                if (!IsPebble(b)) continue;
                pebHits[(int)b]++;
                checked++;

                if (b == BlockType::StonePebble) continue;
                bool ore = (b == BlockType::CoalPebble || b == BlockType::IronPebble ||
                    b == BlockType::CopperPebble || b == BlockType::TinPebble ||
                    b == BlockType::ZincPebble || b == BlockType::GoldPebble ||
                    b == BlockType::SilverPebble || b == BlockType::DiamondPebble);
                if (ore) { orePeb++; continue; }
                rockPeb++;

                BlockType host = gen.GetSurfaceRock(x, z);
                BlockType expect = BlockType::StonePebble;
                switch (host) {
                case BlockType::Granite: case BlockType::Diorite: case BlockType::Gabbro:
                    expect = BlockType::GranitePebble; break;
                case BlockType::Basalt: case BlockType::Andesite:
                case BlockType::Rhyolite: case BlockType::Dacite:
                    expect = BlockType::BasaltPebble; break;
                case BlockType::Limestone: case BlockType::Dolomite:
                case BlockType::Chalk: case BlockType::Marble:
                    expect = BlockType::LimestonePebble; break;
                case BlockType::Sandstone: case BlockType::RedSandstone:
                case BlockType::Conglomerate: case BlockType::Claystone:
                    expect = BlockType::SandstonePebble; break;
                case BlockType::Chert: case BlockType::Shale: case BlockType::Slate:
                    expect = BlockType::FlintPebble; break;
                case BlockType::Gneiss: case BlockType::Schist: case BlockType::Phyllite:
                    expect = BlockType::GranitePebble; break;
                default: expect = BlockType::StonePebble; break;
                }
                if (expect != BlockType::StonePebble && b != expect) mismatch++;
            }
        }
        printf("  %ld pebbles: %ld rock, %ld ore\n", checked, rockPeb, orePeb);
        for (auto& kv : pebHits)
            printf("    %-20s %6ld  %5.1f%%\n", BlockInfo::GetName(kv.first).c_str(),
                kv.second, 100.0 * kv.second / std::max(1L, checked));
        if (mismatch > 0) {
            printf("  FAIL %ld rock pebbles do not match the rock beneath\n", mismatch);
            fails++;
        }
        else printf("  every rock pebble matches its bedrock\n");
        if (checked == 0) { printf("  FAIL no pebbles generated at all\n"); fails++; }
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
