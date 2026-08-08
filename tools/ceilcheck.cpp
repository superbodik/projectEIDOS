#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);

    printf("=== skipping empty sky must not change a single block ===\n");
    printf("    full 0..255 fill compared against the ceiling shortcut\n\n");

    long long compared = 0, diffs = 0;
    int firstX = 0, firstY = 0, firstZ = 0;
    long long saved = 0, totalCells = 0;

    const int SPAN = 260;

    for (int x = -SPAN; x <= SPAN; x += 7) {
        for (int z = -SPAN; z <= SPAN; z += 7) {
            ColumnInfo col = gen.GetColumnInfo(x, z);

            int ceiling = std::max(col.height + 24, WorldGenerator::SEA_LEVEL);
            ceiling = std::max(ceiling, col.river.fallTop);
            ceiling = std::max(ceiling, col.river.waterLevel);
            if (ceiling > 255) ceiling = 255;

            totalCells += 256;
            saved += 255 - ceiling;

            for (int y = 0; y < 256; y++) {
                BlockType full = gen.GetBlockFast(x, y, z, col);
                BlockType fast = (y <= ceiling)
                    ? gen.GetBlockFast(x, y, z, col)
                    : BlockType::Air;
                compared++;
                if (full != fast) {
                    if (diffs == 0) { firstX = x; firstY = y; firstZ = z; }
                    diffs++;
                }
            }
        }
    }

    printf("  %lld blocks compared\n", compared);
    printf("  %lld differences\n", diffs);
    if (diffs) {
        printf("  FAIL first at x=%d y=%d z=%d\n", firstX, firstY, firstZ);
        printf("       a real block was cut off above the ceiling\n");
        printf("\nFAILED\n");
        fflush(stdout);
        return 1;
    }

    printf("  %.1f%% of the column is provably empty sky and is now skipped\n",
        100.0 * saved / (totalCells ? totalCells : 1));
    printf("\nALL CHECKS PASSED (0 failures)\n");
    fflush(stdout);
    return 0;
}
