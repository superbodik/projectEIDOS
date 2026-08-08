#include "../src/World/WorldGenerator.h"
#include "../src/World/Chunk.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);
    int fails = 0;

    const BlockType NEW_VEINS[] = {
        BlockType::Barite, BlockType::Fluorite, BlockType::Phosphorite,
        BlockType::Sylvite, BlockType::Wolframite, BlockType::Uraninite
    };
    const BlockType NEW_PEBBLES[] = {
        BlockType::LeadPebble, BlockType::BaritePebble, BlockType::FluoritePebble,
        BlockType::PhosphoritePebble, BlockType::PotashPebble, BlockType::TungstenPebble,
        BlockType::UraniumPebble
    };
    const BlockType ALL_PEBBLES[] = {
        BlockType::StonePebble, BlockType::CopperPebble, BlockType::IronPebble,
        BlockType::CoalPebble, BlockType::GoldPebble, BlockType::DiamondPebble,
        BlockType::FlintPebble, BlockType::GranitePebble, BlockType::BasaltPebble,
        BlockType::LimestonePebble, BlockType::SandstonePebble, BlockType::TinPebble,
        BlockType::SilverPebble, BlockType::ZincPebble,
        BlockType::LeadPebble, BlockType::BaritePebble, BlockType::FluoritePebble,
        BlockType::PhosphoritePebble, BlockType::PotashPebble, BlockType::TungstenPebble,
        BlockType::UraniumPebble
    };

    printf("=== new ore veins reachable underground ===\n");
    {
        std::map<BlockType, long> found;
        for (int x = -4000; x <= 4000; x += 5) {
            for (int z = -4000; z <= 4000; z += 5) {
                ColumnInfo col = gen.GetColumnInfo(x, z);
                for (int y = 2; y < col.height; y += 3) {
                    BlockType b = gen.GetBlockFast(x, y, z, col);
                    for (BlockType v : NEW_VEINS) if (b == v) found[v]++;
                }
            }
        }
        for (BlockType v : NEW_VEINS) {
            long n = found[v];
            printf("  %-14s %6ld blocks\n", BlockInfo::GetName((int)v).c_str(), n);
            if (n == 0) { printf("  FAIL unreachable\n"); fails++; }
        }
    }

    printf("\n=== new pebbles reachable on the surface ===\n");
    {
        std::map<BlockType, long> found;
        for (int x = -6000; x <= 6000; x += 2) {
            for (int z = -6000; z <= 6000; z += 2) {
                ColumnInfo col = gen.GetColumnInfo(x, z);
                BlockType b = gen.GetBlockFast(x, col.height + 1, z, col);
                for (BlockType p : NEW_PEBBLES) if (b == p) found[p]++;
            }
        }
        for (BlockType p : NEW_PEBBLES) {
            long n = found[p];
            printf("  %-22s %6ld\n", BlockInfo::GetName((int)p).c_str(), n);
            if (n == 0) { printf("  FAIL unreachable\n"); fails++; }
        }
    }

    printf("\n=== every pebble has its own unique, painted atlas tile ===\n");
    {
        Image atlas = Chunk::BuildAtlasImage();
        std::map<int, BlockType> byCell;

        for (BlockType p : ALL_PEBBLES) {
            float u = 0, v = 0;
            Chunk::GetTextureUV(p, 2, u, v);
            int col = (int)((u + 0.001f) / 0.0625f);
            int row = (int)((v + 0.001f) / 0.0625f);
            int cell = row * 16 + col;

            if (byCell.count(cell)) {
                printf("  FAIL %s shares a tile with %s (col=%d row=%d)\n",
                    BlockInfo::GetName((int)p).c_str(),
                    BlockInfo::GetName((int)byCell[cell]).c_str(), col, row);
                fails++;
            }
            byCell[cell] = p;

            int px = col * 16 + 8, py = row * 16 + 8;
            Color c = GetImageColor(atlas, px, py);
            if (c.a < 200) {
                printf("  FAIL %s tile is transparent (col=%d row=%d, alpha=%d)\n",
                    BlockInfo::GetName((int)p).c_str(), col, row, c.a);
                fails++;
            }
        }
        printf("  %d pebble types checked\n", (int)(sizeof(ALL_PEBBLES) / sizeof(ALL_PEBBLES[0])));
        UnloadImage(atlas);
    }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    fflush(stdout);
    std::_Exit(fails ? 1 : 0);
}
