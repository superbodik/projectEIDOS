#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <map>
#include <set>
#include <string>

int main() {
    WorldGenerator gen(1337);
    int fails = 0;

    const int SPAN = 1800;

    printf("=== how ragged are biome borders ===\n");
    printf("    a hard border runs straight for many blocks;\n");
    printf("    a blended one dithers back and forth\n\n");

    long borders = 0;
    long straightRuns = 0;
    long longestRun = 0;

    for (int z = -SPAN; z <= SPAN; z += 3) {
        BiomeType prev = gen.GetBiome(-SPAN, z);
        long run = 0;

        for (int x = -SPAN + 1; x <= SPAN; x++) {
            BiomeType cur = gen.GetBiome(x, z);
            if (cur == prev) { run++; continue; }

            borders++;
            prev = cur;

            int flips = 0;
            BiomeType a = cur;
            for (int k = 1; k <= 12; k++) {
                BiomeType b = gen.GetBiome(x + k, z);
                if (b != a) { flips++; a = b; }
            }
            if (flips == 0) straightRuns++;
            if (run > longestRun) longestRun = run;
            run = 0;
        }
    }

    double hardPct = 100.0 * straightRuns / (borders ? borders : 1);
    printf("  %ld borders crossed\n", borders);
    printf("  %ld of them are a clean single cut (%.1f%%)\n", straightRuns, hardPct);
    printf("  longest unbroken biome run: %ld blocks\n", longestRun);

    if (hardPct > 55.0) {
        printf("  FAIL most borders are hard cuts, not blended\n");
        fails++;
    }

    printf("\n=== every biome still reachable after blending ===\n");
    {
        std::set<int> seen;
        for (int x = -6000; x <= 6000; x += 40)
            for (int z = -6000; z <= 6000; z += 40)
                seen.insert((int)gen.GetBiome(x, z));
        printf("  %d distinct biomes found in a 12000x12000 sweep\n", (int)seen.size());
        if (seen.size() < 20) {
            printf("  FAIL blending collapsed the biome variety\n");
            fails++;
        }
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
