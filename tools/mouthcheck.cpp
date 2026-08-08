#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <cmath>

static bool IsWater(BlockType b) {
    return b == BlockType::Water || b == BlockType::WaterSource ||
        b == BlockType::Ice;
}

int main() {
    WorldGenerator gen(1337);
    int fails = 0;
    const int SEA = WorldGenerator::SEA_LEVEL;

    printf("=== how low does a river reach ===\n");
    int lowest = 999;
    long channels = 0;
    long atSea = 0;

    for (int x = -3000; x <= 3000; x += 3) {
        for (int z = -3000; z <= 3000; z += 3) {
            ColumnInfo c = gen.GetColumnInfo(x, z);
            if (!c.river.channel) continue;
            channels++;
            if (c.river.waterLevel < lowest) lowest = c.river.waterLevel;
            if (c.river.waterLevel <= SEA) atSea++;
        }
    }
    printf("  %ld channel columns, lowest water level y=%d (sea is y=%d)\n",
        channels, lowest, SEA);
    printf("  %ld of them sit at sea level (%.1f%%)\n",
        atSea, 100.0 * atSea / (channels ? channels : 1));

    if (channels == 0) { printf("  FAIL no rivers at all\n"); return 1; }
    if (lowest > SEA) {
        printf("  FAIL rivers stop above the sea and never reach it\n");
        fails++;
    }
    if (atSea == 0) {
        printf("  FAIL no river column reaches sea level\n");
        fails++;
    }

    printf("=== river mouths actually touch open water ===\n");
    {
        long mouths = 0, connected = 0;

        for (int x = -3000; x <= 3000; x += 3) {
            for (int z = -3000; z <= 3000; z += 3) {
                ColumnInfo c = gen.GetColumnInfo(x, z);
                if (!c.river.channel) continue;
                if (c.river.waterLevel > SEA) continue;
                mouths++;

                bool touches = false;
                const int dx[8] = { 6, -6, 0, 0, 5, 5, -5, -5 };
                const int dz[8] = { 0, 0, 6, -6, 5, -5, 5, -5 };
                for (int k = 0; k < 8 && !touches; k++) {
                    int nx = x + dx[k], nz = z + dz[k];
                    ColumnInfo n = gen.GetColumnInfo(nx, nz);
                    if (n.height < SEA) { touches = true; break; }
                    BlockType b = gen.GetBlockFast(nx, SEA, nz, n);
                    if (IsWater(b)) touches = true;
                }
                if (touches) connected++;
            }
        }

        double pct = 100.0 * connected / (mouths ? mouths : 1);
        printf("  %ld sea-level river columns, %ld touch open water (%.1f%%)\n",
            mouths, connected, pct);
        if (mouths > 0 && pct < 60.0) {
            printf("  FAIL river mouths end in dry land\n");
            fails++;
        }
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
