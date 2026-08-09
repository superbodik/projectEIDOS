#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <cstdlib>
#include <map>

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);
    int fails = 0;

    printf("=== fallen sticks by biome ===\n");
    std::map<std::string, long> sticksByBiome;
    std::map<std::string, long> columnsByBiome;

    for (int x = -4000; x <= 4000; x += 3) {
        for (int z = -4000; z <= 4000; z += 3) {
            std::string name = gen.GetBiomeName(x, z);
            if (name.empty() || name == "River" || name == "Unknown") continue;
            ColumnInfo col = gen.GetColumnInfo(x, z);
            columnsByBiome[name]++;
            BlockType b = gen.GetBlockFast(x, col.height + 1, z, col);
            if (b == BlockType::Stick) sticksByBiome[name]++;
        }
    }

    for (auto& [name, cols] : columnsByBiome) {
        long sticks = sticksByBiome.count(name) ? sticksByBiome[name] : 0;
        if (sticks > 0)
            printf("  %-24s %6ld sticks / %7ld columns (%.3f%%)\n",
                name.c_str(), sticks, cols, 100.0 * sticks / cols);
    }

    double forestRate = 100.0 * sticksByBiome["Forest"] / columnsByBiome["Forest"];
    double desertRate = columnsByBiome.count("Desert")
        ? 100.0 * sticksByBiome["Desert"] / columnsByBiome["Desert"] : 0.0;
    double scorchedRate = columnsByBiome.count("Scorched Desert") && sticksByBiome.count("Scorched Desert")
        ? 100.0 * sticksByBiome["Scorched Desert"] / columnsByBiome["Scorched Desert"] : 0.0;

    printf("\n  Forest rate:   %.3f%%\n", forestRate);
    printf("  Desert rate:   %.3f%% (Savanna, which has trees, borders Desert - some bleed is real)\n", desertRate);
    printf("  Scorched rate: %.3f%% (should be ~0, nothing borders it with trees)\n", scorchedRate);

    if (forestRate == 0.0) { printf("  FAIL no sticks in Forest\n"); fails++; }
    if (desertRate > forestRate / 5.0) {
        printf("  FAIL desert stick rate too close to forest - not just border bleed\n");
        fails++;
    }
    if (scorchedRate > 0.0) { printf("  FAIL sticks in scorched desert, nothing to bleed from\n"); fails++; }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    fflush(stdout);
    std::_Exit(fails ? 1 : 0);
}
