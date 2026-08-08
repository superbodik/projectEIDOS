#include "../src/World/WorldGenerator.h"
#include "../src/Core/WorldRules.h"
#include <cstdio>
#include <cmath>

struct Tally { long ore = 0; long cave = 0; long tree = 0; long stone = 0; };

static Tally Sample(WorldGenerator& gen) {
    Tally t;
    for (int x = -400; x <= 400; x += 11) {
        for (int z = -400; z <= 400; z += 11) {
            ColumnInfo c = gen.GetColumnInfo(x, z);
            for (int y = 6; y < c.height; y += 3) {
                BlockType b = gen.GetBlockFast(x, y, z, c);
                int id = (int)b;
                if (b == BlockType::Air) { t.cave++; continue; }
                if (id >= 50 && id <= 66) { t.ore++; t.stone++; }
                else if (id >= 16 && id <= 45) t.stone++;
            }
            for (int y = c.height + 1; y < c.height + 14; y++) {
                int id = (int)gen.GetBlockFast(x, y, z, c);
                if (id >= 100 && id <= 109) { t.tree++; break; }
            }
        }
    }
    return t;
}

int main() {
    int fails = 0;

    printf("=== defaults must not change the world ===\n");
    {
        WorldGenerator a(1337);
        WorldGenerator b(1337);
        WorldGenerator::GenSettings g;
        b.SetGenSettings(g);

        long diff = 0, checked = 0;
        for (int x = -300; x <= 300; x += 7) {
            for (int z = -300; z <= 300; z += 7) {
                ColumnInfo ca = a.GetColumnInfo(x, z);
                ColumnInfo cb = b.GetColumnInfo(x, z);
                if (ca.height != cb.height) diff++;
                for (int y = 8; y < ca.height; y += 9) {
                    checked++;
                    if (a.GetBlockFast(x, y, z, ca) != b.GetBlockFast(x, y, z, cb)) diff++;
                }
            }
        }
        printf("  %ld blocks compared, %ld differences\n", checked, diff);
        if (diff != 0) { printf("  FAIL default settings altered generation\n"); fails++; }
    }

    printf("=== multipliers actually do something ===\n");
    WorldGenerator base(4242);
    Tally t0 = Sample(base);
    printf("  baseline: ore %ld / stone %ld (%.2f%%), caves %ld, trees %ld\n",
        t0.ore, t0.stone, 100.0 * t0.ore / (t0.stone ? t0.stone : 1), t0.cave, t0.tree);

    struct Case { const char* name; WorldGenerator::GenSettings g; };
    WorldGenerator::GenSettings gOre;  gOre.oreDensity = 3.0f;
    WorldGenerator::GenSettings gNoCave; gNoCave.caveDensity = 0.0f;

    {
        WorldGenerator g(4242); g.SetGenSettings(gOre);
        Tally t = Sample(g);
        double b0 = 100.0 * t0.ore / (t0.stone ? t0.stone : 1);
        double b1 = 100.0 * t.ore / (t.stone ? t.stone : 1);
        printf("  oreDensity 3.0 -> %.2f%% (was %.2f%%)\n", b1, b0);
        if (b1 <= b0 * 1.8) { printf("  FAIL ore density had little effect\n"); fails++; }
    }
    {
        WorldGenerator g(4242); g.SetGenSettings(gNoCave);
        Tally t = Sample(g);
        printf("  caveDensity 0.0 -> %ld air pockets (was %ld)\n", t.cave, t0.cave);
        if (t.cave >= t0.cave) { printf("  FAIL caves were not removed\n"); fails++; }
    }
    {
        WorldGenerator::GenSettings gLat; gLat.latitudeScale = 1500.0f;
        WorldGenerator g(4242); g.SetGenSettings(gLat);
        float warm = g.GetTemperature(0, 0);
        float cold = g.GetTemperature(0, 1400);
        printf("  latitudeScale 1500 -> temp at z=0 %.2f, at z=1400 %.2f\n", warm, cold);
        if (warm - cold < 0.30f) { printf("  FAIL latitude scale had no effect\n"); fails++; }
    }

    printf("=== rules survive a save/load round trip ===\n");
    {
        WorldRules r = WorldRules::Preset(Difficulty::Hardcore);
        r.oneLife = true;
        r.caveDensity = 0.4f;
        r.latitudeScale = 2500.0f;

        WorldRules back;
        if (!back.Deserialize(r.Serialize())) {
            printf("  FAIL could not parse own output\n"); fails++;
        }
        else if (back.difficulty != r.difficulty || !back.oneLife ||
            std::fabs(back.caveDensity - 0.4f) > 0.001f ||
            std::fabs(back.latitudeScale - 2500.0f) > 0.1f ||
            std::fabs(back.regenFloor - r.regenFloor) > 0.001f) {
            printf("  FAIL values changed through the round trip\n"); fails++;
        }
        else printf("  hardcore + oneLife + custom generation survived\n");

        WorldRules old;
        if (old.Deserialize("garbage line")) {
            printf("  FAIL accepted a malformed line\n"); fails++;
        }
        else printf("  malformed line rejected, defaults kept\n");
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
