#include "../src/World/WorldGenerator.h"
#include "../src/Inventory/FoodSystem.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <map>
#include <string>

int main() {
    WorldGenerator gen(1337);
    int fails = 0;

    printf("=== nutrient sources ===\n");
    bool covered[4] = { false, false, false, false };
    for (const Food::Def& d : Food::All()) {
        printf("  %-16s sat %+.2f  hyd %+.2f  %-11s %+.0f%%%s\n",
            d.name, d.satiety, d.hydration, Food::NutrientName(d.nutrient),
            d.amount * 100.0f, d.poison > 0.0f ? "   POISON" : "");
        if (d.nutrient >= 0 && d.nutrient < 4 && d.poison <= 0.0f) covered[d.nutrient] = true;
    }
    for (int i = 0; i < 4; i++) {
        if (!covered[i]) {
            printf("  FAIL no Era I source for %s - nutrient can only decay\n",
                Food::NutrientName(i));
            fails++;
        }
    }
    if (fails == 0) printf("  all four nutrient groups have a forageable source\n");

    printf("=== every food has a name ===\n");
    for (const Food::Def& d : Food::All()) {
        std::string n = BlockInfo::GetName(d.id);
        if (n.empty() || n == "Unknown" || n.rfind("Block ", 0) == 0) {
            printf("  FAIL id %d (%s) has no display name (got \"%s\")\n", d.id, d.name, n.c_str());
            fails++;
        }
    }
    printf("  ok\n");

    printf("=== berry bush generation ===\n");
    std::map<std::string, long> bushByBiome;
    std::map<std::string, long> colsByBiome;
    long bushes = 0, ripe = 0, columns = 0;

    for (int x = -2600; x <= 2600; x += 7) {
        for (int z = -2600; z <= 2600; z += 7) {
            ColumnInfo col = gen.GetColumnInfo(x, z);
            std::string bn = gen.GetBiomeName(x, z);
            colsByBiome[bn]++;
            columns++;

            BlockType b = gen.GetBlockFast(x, col.height + 1, z, col);
            if (b == BlockType::BerryBush || b == BlockType::BerryBushRipe) {
                bushes++;
                bushByBiome[bn]++;
                if (b == BlockType::BerryBushRipe) ripe++;
            }
        }
    }

    double per1k = 1000.0 * bushes / (double)columns;
    printf("  %ld bushes over %ld columns (%.2f per 1000 surface blocks)\n",
        bushes, columns, per1k);
    printf("  %ld ripe on generation (%.0f%%)\n", ripe,
        bushes ? 100.0 * ripe / bushes : 0.0);

    for (auto& kv : bushByBiome) {
        double d = 1000.0 * kv.second / (double)std::max(1L, colsByBiome[kv.first]);
        printf("    %-22s %6ld  (%.2f/1k)\n", kv.first.c_str(), kv.second, d);
    }

    if (bushes == 0) {
        printf("  FAIL berry bushes never generate - fruit is unobtainable\n");
        fails++;
    }
    else if (per1k < 0.15) {
        printf("  FAIL too rare to find (want >= 0.15 per 1000)\n");
        fails++;
    }
    else if (per1k > 25.0) {
        printf("  FAIL berries everywhere - no scarcity\n");
        fails++;
    }

    printf("=== bushes stand on soil ===\n");
    {
        long checked = 0, bad = 0;
        for (int x = -1500; x <= 1500; x += 3) {
            for (int z = -1500; z <= 1500; z += 3) {
                ColumnInfo col = gen.GetColumnInfo(x, z);
                BlockType b = gen.GetBlockFast(x, col.height + 1, z, col);
                if (b != BlockType::BerryBush && b != BlockType::BerryBushRipe) continue;
                checked++;
                BlockType ground = gen.GetBlockFast(x, col.height, z, col);
                if (ground != BlockType::Grass && ground != BlockType::Dirt &&
                    ground != BlockType::CoarseDirt) {
                    if (bad < 5)
                        printf("    bush at %d,%d sits on %s\n", x, z,
                            BlockInfo::GetName((int)ground).c_str());
                    bad++;
                }
            }
        }
        printf("  %ld checked, %ld on wrong ground\n", checked, bad);
        if (checked > 0 && bad * 100 > checked * 5) {
            printf("  FAIL more than 5%% of bushes would rot away on the first tick\n");
            fails++;
        }
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
