#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <cmath>
#include <chrono>

int main() {
    WorldGenerator gen(1337);
    int fails = 0;

    const char* probes[] = {
        "Desert", "desert", "DESERT", "  desert ", "Snowy Tundra", "snowy_tundra",
        "SNOWY-TUNDRA", "ice spikes", "IceSpikes", "jungle", "Tropical Rainforest",
        "dry plains", "Dry_Plains", "forest", "Deciduous Forest", "rainforest",
        "taiga", "boreal forest", "mountains", "peaks", "river", "RIVER",
        "beach", "shore", "ocean", "sea", "scorched", "badlands", "savannah"
    };
    printf("=== ParseBiome ===\n");
    for (const char* p : probes) {
        BiomeType t;
        bool ok = WorldGenerator::ParseBiome(p, t);
        if (!ok) { printf("  FAIL  \"%s\" not recognised\n", p); fails++; }
    }
    printf("  %d/%d aliases accepted\n", (int)(sizeof(probes) / sizeof(*probes)) - fails,
        (int)(sizeof(probes) / sizeof(*probes)));

    BiomeType dummy;
    const char* junk[] = { "", "qwerty", "desrt", "biome", "12345" };
    printf("=== rejects junk ===\n");
    for (const char* j : junk) {
        if (WorldGenerator::ParseBiome(j, dummy)) {
            printf("  FAIL  \"%s\" wrongly accepted\n", j);
            fails++;
        }
    }
    printf("  ok\n");

    printf("=== FindBiome from (0,0) ===\n");
    for (const std::string& name : WorldGenerator::BiomeNames()) {
        BiomeType t;
        if (!WorldGenerator::ParseBiome(name, t)) {
            printf("  %-18s CANONICAL NAME UNPARSEABLE\n", name.c_str());
            fails++;
            continue;
        }
        int step = (t == BiomeType::River || t == BiomeType::Beach) ? 16 : 64;
        auto t0 = std::chrono::steady_clock::now();
        BiomeSearchResult r = gen.FindBiome(0, 0, t, 32000, step);
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        if (!r.found) {
            printf("  %-18s NOT FOUND within 32000  (%.0f ms)\n", name.c_str(), ms);
            fails++;
            continue;
        }
        int d = (int)sqrtf((float)(r.x * r.x + r.z * r.z));
        std::string got = gen.GetBiomeName(r.x, r.z);
        printf("  %-18s at %7d,%7d  dist=%6d  %6.0f ms  reports \"%s\"\n",
            name.c_str(), r.x, r.z, d, ms, got.c_str());
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL OK", fails);
    return fails ? 1 : 0;
}
