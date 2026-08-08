#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <vector>

static bool IsWater(BlockType b) { return b == BlockType::Water || b == BlockType::Ice; }
static bool IsAir(BlockType b) { return b == BlockType::Air; }

int main(int argc, char** argv) {
    int seed = (argc > 1) ? atoi(argv[1]) : 1337;
    int span = (argc > 2) ? atoi(argv[2]) : 900;
    WorldGenerator gen(seed);

    long long chan = 0, holes = 0, leaks = 0, dryBed = 0, floaters = 0, plunges = 0;
    long long deepest = 0, tallestFall = 0;
    int leakX = 0, leakZ = 0, leakY = 0, holeX = 0, holeZ = 0;

    for (int z = -span; z <= span; z++) {
        for (int x = -span; x <= span; x++) {
            ColumnInfo c = gen.GetColumnInfo(x, z);
            if (!c.river.channel) continue;
            chan++;
            if (c.river.plunge) plunges++;
            int wl = c.river.waterLevel;
            if (wl - c.height > deepest) deepest = wl - c.height;
            if (c.river.fallTop - wl > tallestFall) tallestFall = c.river.fallTop - wl;

            if (!c.river.sill && !IsWater(gen.GetBlockFast(x, wl, z, c))) {
                if (!holes) { holeX = x; holeZ = z; }
                holes++;
            }

            if (IsAir(gen.GetBlockFast(x, c.height, z, c))) dryBed++;

            ColumnInfo n[4] = { gen.GetColumnInfo(x + 1,z), gen.GetColumnInfo(x - 1,z),
                                gen.GetColumnInfo(x,z + 1), gen.GetColumnInfo(x,z - 1) };
            int nx[4] = { x + 1,x - 1,x,x }, nz[4] = { z,z,z + 1,z - 1 };
            for (int y = c.height + 1; y <= wl; y++) {
                if (!IsWater(gen.GetBlockFast(x, y, z, c))) continue;
                for (int i = 0; i < 4; i++) {
                    if (IsAir(gen.GetBlockFast(nx[i], y, nz[i], n[i]))) {
                        if (!leaks) { leakX = x; leakZ = z; leakY = y; }
                        if (leaks < 6)
                            printf("  leak (%d,%d,y=%d) self[wl=%d h=%d sill=%d str=%.2f]"
                                   " nb(%+d,%+d)[act=%d chan=%d wl=%d h=%d sill=%d fall=%d str=%.2f]\n",
                                   x, z, y, wl, c.height, c.river.sill, c.river.strength,
                                   nx[i] - x, nz[i] - z, n[i].river.active, n[i].river.channel,
                                   n[i].river.waterLevel, n[i].height, n[i].river.sill,
                                   n[i].river.fallTop, n[i].river.strength);
                        leaks++;
                        break;
                    }
                }
            }

            for (int y = c.height + 1; y <= wl; y++)
                if (IsWater(gen.GetBlockFast(x, y, z, c)) &&
                    IsAir(gen.GetBlockFast(x, y - 1, z, c))) floaters++;
        }
    }

    printf("seed %d, %d x %d blocks\n", seed, span * 2 + 1, span * 2 + 1);
    printf("  channel columns : %lld\n", chan);
    printf("  plunge columns  : %lld\n", plunges);
    printf("  max pool depth  : %lld blocks\n", deepest);
    printf("  tallest fall    : %lld blocks\n", tallestFall);
    printf("  surface holes   : %lld%s\n", holes,
           holes ? "  <-- DEFECT" : "");
    if (holes) printf("      first at (%d,%d)\n", holeX, holeZ);
    printf("  dry bed         : %lld%s\n", dryBed, dryBed ? "  <-- DEFECT" : "");
    printf("  side leaks      : %lld%s\n", leaks, leaks ? "  <-- DEFECT" : "");
    if (leaks) printf("      first at (%d,%d,y=%d)\n", leakX, leakZ, leakY);
    printf("  floating water  : %lld%s\n", floaters, floaters ? "  <-- DEFECT" : "");
    return (holes || leaks || dryBed || floaters) ? 1 : 0;
}
