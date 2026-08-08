#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <map>
#include <string>

int main(int argc, char** argv) {
    int seed = (argc > 1) ? atoi(argv[1]) : 1337;
    int ox = (argc > 2) ? atoi(argv[2]) : 0;
    int oz = (argc > 3) ? atoi(argv[3]) : 0;
    int step = (argc > 4) ? atoi(argv[4]) : 8;
    WorldGenerator gen(seed);

    const int W = 118, H = 54;
    printf("=== seed %d  origin (%d,%d)  step %d  span %dx%d blocks ===\n",
           seed, ox, oz, step, W * step, H * step);

    long long chanCols = 0, bankCols = 0, fallCols = 0, total = 0;
    for (int r = 0; r < H; r++) {
        std::string line;
        for (int c = 0; c < W; c++) {
            int x = ox + c * step, z = oz + r * step;
            ColumnInfo ci = gen.GetColumnInfo(x, z);
            total++;
            char ch;
            if (ci.river.channel) {
                chanCols++;
                if (ci.river.fallTop > ci.river.waterLevel) { ch = '#'; fallCols++; }
                else ch = '~';
            } else if (ci.river.active) { bankCols++; ch = '.'; }
            else if (ci.height <= 60)   ch = ' ';
            else if (ci.height < 70)    ch = '-';
            else if (ci.height < 95)    ch = '+';
            else if (ci.height < 125)   ch = '^';
            else                        ch = 'A';
            line += ch;
        }
        printf("%s\n", line.c_str());
    }
    printf("\nchannel=%lld (%.2f%%)  bank=%lld  waterfall=%lld  of %lld sampled cols\n",
           chanCols, 100.0 * chanCols / total, bankCols, fallCols, total);

    int bx = 0, bz = 0; float bestW = 0;
    for (int r = 0; r < H * 4; r++) for (int c = 0; c < W * 4; c++) {
        int x = ox + c * 2, z = oz + r * 2;
        ColumnInfo ci = gen.GetColumnInfo(x, z);
        if (ci.river.channel && ci.river.strength > bestW) { bestW = ci.river.strength; bx = x; bz = z; }
    }
    printf("\n=== cross-section at (%d,%d), X sweep ===\n", bx, bz);
    ColumnInfo mid = gen.GetColumnInfo(bx, bz);
    int wl = mid.river.waterLevel;
    printf("waterLevel=%d fallTop=%d height=%d strength=%.2f\n",
           wl, mid.river.fallTop, mid.height, mid.river.strength);
    for (int y = wl + 6; y >= wl - 12; y--) {
        std::string line;
        for (int d = -30; d <= 30; d++) {
            ColumnInfo ci = gen.GetColumnInfo(bx + d, bz);
            BlockType b = gen.GetBlockFast(bx + d, y, bz, ci);
            char ch = ' ';
            switch (b) {
            case BlockType::Air: ch = ' '; break;
            case BlockType::Water: ch = '~'; break;
            case BlockType::Ice: ch = '='; break;
            case BlockType::Sand: ch = 's'; break;
            case BlockType::Gravel: ch = 'g'; break;
            case BlockType::Clay: ch = 'c'; break;
            case BlockType::Grass: ch = 'G'; break;
            case BlockType::Dirt: ch = 'd'; break;
            case BlockType::Stone: ch = 'S'; break;
            case BlockType::TallGrass: ch = 'v'; break;
            default: ch = '?'; break;
            }
            line += ch;
        }
        printf("y=%3d |%s|\n", y, line.c_str());
    }

    printf("\n=== long profile along Z at x=%d ===\n", bx);
    for (int z = bz - 60; z <= bz + 60; z += 4) {
        ColumnInfo ci = gen.GetColumnInfo(bx, z);
        printf("z=%6d h=%3d %s wl=%3d fall=%3d str=%.2f%s\n", z, ci.height,
               ci.river.channel ? "CHAN" : (ci.river.active ? "bank" : "    "),
               ci.river.waterLevel, ci.river.fallTop, ci.river.strength,
               ci.river.plunge ? "  <plunge>" : "");
    }
    return 0;
}
