#include "../src/World/WorldGenerator.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <map>
#include <string>

static bool IsPlant(int id) {
    if (id >= 110 && id <= 117) return true;
    if (id == 123 || id == 125 || id == 126) return true;
    if (id == 128 || id == 129) return true;
    if (id >= 170 && id <= 179) return true;
    return false;
}

static bool IsSoil(BlockType b) {
    return b == BlockType::Grass || b == BlockType::Dirt ||
        b == BlockType::CoarseDirt || b == BlockType::Mud ||
        b == BlockType::Peat || b == BlockType::Sand ||
        b == BlockType::RedSand;
}

static bool IsMountainBiome(BiomeType b) {
    return b == BiomeType::Mountains || b == BiomeType::Volcano ||
        b == BiomeType::Crag || b == BiomeType::Glacier ||
        b == BiomeType::AlpineMeadow;
}

int main() {
    WorldGenerator gen(1337);
    int fails = 0;

    const int SPAN = 2600, STEP = 5;

    printf("=== plants only stand on soil ===\n");
    {
        long plants = 0, wrong = 0, floating = 0;
        std::map<std::string, long> offenders;

        for (int x = -SPAN; x <= SPAN; x += STEP) {
            for (int z = -SPAN; z <= SPAN; z += STEP) {
                ColumnInfo c = gen.GetColumnInfo(x, z);
                if (c.height < 1) continue;

                int id = (int)gen.GetBlockFast(x, c.height + 1, z, c);
                if (!IsPlant(id)) continue;
                plants++;

                BlockType ground = gen.GetBlockFast(x, c.height, z, c);
                if (IsSoil(ground)) continue;

                if (ground == BlockType::Air) {
                    floating++;
                    continue;
                }
                wrong++;
                offenders[BlockInfo::GetName((int)ground)]++;
            }
        }
        printf("  %ld plants checked\n", plants);
        printf("    on bare rock or ice : %ld\n", wrong);
        printf("    floating over air   : %ld  (%.3f%%, cave mouths)\n",
            floating, 100.0 * floating / (plants ? plants : 1));
        for (auto& kv : offenders)
            printf("    on %-18s %ld\n", kv.first.c_str(), kv.second);

        if (wrong > 0) { printf("  FAIL plants grow on bare rock\n"); fails++; }
        if (floating * 500 > plants) {
            printf("  FAIL too many plants hang over carved ground\n");
            fails++;
        }
    }

    printf("=== mountain biomes sit on high ground ===\n");
    {
        long mountains = 0, tooLow = 0;
        std::map<std::string, long> lowOnes;

        for (int x = -SPAN; x <= SPAN; x += STEP) {
            for (int z = -SPAN; z <= SPAN; z += STEP) {
                BiomeType b = gen.GetBiome(x, z);
                if (!IsMountainBiome(b)) continue;
                mountains++;

                int h = gen.GetHeight(x, z);
                if (h <= WorldGenerator::SEA_LEVEL + 2) {
                    tooLow++;
                    lowOnes[gen.GetBiomeName(x, z)]++;
                }
            }
        }
        printf("  %ld mountain columns, %ld at or below sea level\n", mountains, tooLow);
        for (auto& kv : lowOnes)
            printf("    %-20s %ld\n", kv.first.c_str(), kv.second);
        if (tooLow * 200 > mountains) {
            printf("  FAIL mountain biomes drowning in the sea\n");
            fails++;
        }
    }

    printf("=== lava never sits at the shoreline ===\n");
    {
        long lava = 0, bad = 0;
        for (int x = -SPAN; x <= SPAN; x += STEP) {
            for (int z = -SPAN; z <= SPAN; z += STEP) {
                ColumnInfo c = gen.GetColumnInfo(x, z);
                if (c.height < 1) continue;
                BlockType s = gen.GetBlockFast(x, c.height, z, c);
                if (s != BlockType::Lava) continue;
                lava++;
                if (c.height <= WorldGenerator::SEA_LEVEL + 4) bad++;
            }
        }
        printf("  %ld surface lava, %ld near sea level\n", lava, bad);
        if (bad > 0) { printf("  FAIL lava pooling at the waterline\n"); fails++; }
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
