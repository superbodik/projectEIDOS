#include "../src/World/Chunk.h"
#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <cstdlib>
#include <memory>

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);

    printf("=== nothing may exist above maxY ===\n");
    printf("    the mesher never looks higher, so any block above it\n");
    printf("    is generated and then never drawn\n\n");

    long long chunksTested = 0, hidden = 0, hiddenWater = 0;
    int worstX = 0, worstY = 0, worstZ = 0, worstMax = 0;

    for (int cx = -40; cx <= 40; cx += 3) {
        for (int cz = -40; cz <= 40; cz += 3) {
            auto c = std::make_unique<Chunk>(cx, cz);
            c->GenerateTerrain(gen);
            chunksTested++;

            for (int y = c->maxY + 1; y < Chunk::CHUNK_SIZE_Y; y++) {
                for (int x = 0; x < Chunk::CHUNK_SIZE_X; x++) {
                    for (int z = 0; z < Chunk::CHUNK_SIZE_Z; z++) {
                        BlockType b = c->GetBlock(x, y, z);
                        if (b == BlockType::Air) continue;
                        if (hidden == 0) {
                            worstX = cx * 16 + x; worstY = y;
                            worstZ = cz * 16 + z; worstMax = c->maxY;
                        }
                        hidden++;
                        if (Chunk::IsWaterBlock(b) || b == BlockType::Ice) hiddenWater++;
                    }
                }
            }
        }
    }

    printf("=== what the old formula would have hidden ===\n");
    {
        long long oldHidden = 0, oldWater = 0, badChunks = 0;
        for (int cx = -40; cx <= 40; cx += 3) {
            for (int cz = -40; cz <= 40; cz += 3) {
                int oldMax = 0;
                for (int x = 0; x < 16; x++)
                    for (int z = 0; z < 16; z++) {
                        ColumnInfo col = gen.GetColumnInfo(cx * 16 + x, cz * 16 + z);
                        int topY = col.height > col.river.fallTop
                            ? col.height : col.river.fallTop;
                        if (topY > oldMax) oldMax = topY;
                    }
                oldMax = oldMax + 30;
                if (oldMax > 255) oldMax = 255;

                auto c = std::make_unique<Chunk>(cx, cz);
                c->GenerateTerrain(gen);
                bool bad = false;
                for (int y = oldMax + 1; y < 256; y++)
                    for (int x = 0; x < 16; x++)
                        for (int z = 0; z < 16; z++) {
                            BlockType b = c->GetBlock(x, y, z);
                            if (b == BlockType::Air) continue;
                            oldHidden++;
                            bad = true;
                            if (Chunk::IsWaterBlock(b) || b == BlockType::Ice) oldWater++;
                        }
                if (bad) badChunks++;
            }
        }
        printf("  %lld blocks would have been invisible, %lld of them water\n",
            oldHidden, oldWater);
        printf("  affecting %lld chunks\n\n", badChunks);
    }

    printf("  %lld chunks generated\n", chunksTested);
    printf("  %lld blocks sit above maxY and can never be drawn\n", hidden);
    printf("  of them %lld are water or ice\n", hiddenWater);

    if (hidden) {
        printf("  FAIL first at x=%d y=%d z=%d, chunk maxY=%d\n",
            worstX, worstY, worstZ, worstMax);
        printf("\nFAILED\n");
        fflush(stdout);
        std::_Exit(1);
    }

    printf("\nALL CHECKS PASSED (0 failures)\n");
    fflush(stdout);
    std::_Exit(0);
}
