#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;

static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    WorldGenerator gen(1337);

    const int CHUNKS = 64;
    const int CS = 16;

    printf("=== where world generation spends its time ===\n");
    printf("    (%d chunks, %d columns, %d blocks)\n\n",
        CHUNKS, CHUNKS * CS * CS, CHUNKS * CS * CS * 128);

    double tHeight = 0, tColumn = 0, tBiome = 0, tRock = 0, tBlocks = 0, tDecor = 0;

    for (int ch = 0; ch < CHUNKS; ch++) {
        int ox = (ch % 8) * 16 + 2000;
        int oz = (ch / 8) * 16 - 1500;

        auto t0 = Clock::now();
        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++)
                gen.GetHeight(ox + x, oz + z);
        auto t1 = Clock::now();
        tHeight += ms(t0, t1);

        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++)
                gen.GetBiome(ox + x, oz + z);
        auto t2 = Clock::now();
        tBiome += ms(t1, t2);

        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++)
                gen.GetRockColumn(ox + x, oz + z);
        auto t3 = Clock::now();
        tRock += ms(t2, t3);

        ColumnInfo cols[CS][CS];
        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++)
                cols[x][z] = gen.GetColumnInfo(ox + x, oz + z);
        auto t4 = Clock::now();
        tColumn += ms(t3, t4);

        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++) {
                const ColumnInfo& c = cols[x][z];
                int top = (c.height < 120) ? c.height : 120;
                for (int y = 1; y <= top; y++)
                    gen.GetBlockFast(ox + x, y, oz + z, c);
            }
        auto t5 = Clock::now();
        tBlocks += ms(t4, t5);

        for (int x = 0; x < CS; x++)
            for (int z = 0; z < CS; z++) {
                const ColumnInfo& c = cols[x][z];
                for (int y = c.height + 1; y < c.height + 12; y++)
                    gen.GetBlockFast(ox + x, y, oz + z, c);
            }
        auto t6 = Clock::now();
        tDecor += ms(t5, t6);
    }

    double total = tHeight + tBiome + tRock + tColumn + tBlocks + tDecor;
    struct Row { const char* name; double t; };
    Row rows[] = {
        { "GetHeight        ", tHeight },
        { "GetBiome         ", tBiome  },
        { "GetRockColumn    ", tRock   },
        { "GetColumnInfo    ", tColumn },
        { "solid blocks     ", tBlocks },
        { "decoration/plants", tDecor  },
    };

    for (const Row& r : rows)
        printf("  %s %8.1f ms  %5.1f%%\n", r.name, r.t, 100.0 * r.t / total);

    printf("  %-17s %8.1f ms\n", "TOTAL", total);
    printf("  per chunk        %8.2f ms\n", total / CHUNKS);
    printf("\n  a 17-chunk radius is %d chunks -> %.1f s of generation\n",
        (17 * 2 + 1) * (17 * 2 + 1),
        (total / CHUNKS) * (17 * 2 + 1) * (17 * 2 + 1) / 1000.0);
    printf("  an  8-chunk radius is %d chunks -> %.1f s\n",
        (8 * 2 + 1) * (8 * 2 + 1),
        (total / CHUNKS) * (8 * 2 + 1) * (8 * 2 + 1) / 1000.0);
    return 0;
}
