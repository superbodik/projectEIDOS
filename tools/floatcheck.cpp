#include "../src/World/WorldGenerator.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <map>
#include <vector>
#include <algorithm>

static bool IsAir(BlockType b) { return b == BlockType::Air; }

static bool IsWater(BlockType b) {
    return b == BlockType::Water || b == BlockType::WaterSource;
}

static bool IsPlantLike(BlockType b) {
    int id = (int)b;
    if (id >= 110 && id <= 119) return true;
    if (id >= 123 && id <= 129) return true;
    if (id >= 130 && id <= 149) return true;
    return false;
}

int main() {
    WorldGenerator gen(1337);

    const int SPAN = 900;
    const int STEP = 1;

    long floatingSolid = 0, floatingWater = 0, waterWall = 0, sampled = 0;
    std::map<int, long> floatKinds;
    std::vector<std::string> examples;

    for (int x = -SPAN; x <= SPAN; x += STEP) {
        for (int z = -SPAN; z <= SPAN; z += STEP) {
            ColumnInfo c = gen.GetColumnInfo(x, z);
            ColumnInfo cxp = gen.GetColumnInfo(x + 1, z);
            ColumnInfo cxm = gen.GetColumnInfo(x - 1, z);
            ColumnInfo czp = gen.GetColumnInfo(x, z + 1);
            ColumnInfo czm = gen.GetColumnInfo(x, z - 1);

            int top = std::min(c.height + 26, 200);
            int bottom = std::max(1, c.height - 6);

            for (int y = bottom; y <= top; y++) {
                BlockType b = gen.GetBlockFast(x, y, z, c);
                if (IsAir(b)) continue;
                sampled++;

                BlockType up = gen.GetBlockFast(x, y + 1, z, c);
                BlockType dn = gen.GetBlockFast(x, y - 1, z, c);
                BlockType xp = gen.GetBlockFast(x + 1, y, z, cxp);
                BlockType xm = gen.GetBlockFast(x - 1, y, z, cxm);
                BlockType zp = gen.GetBlockFast(x, y, z + 1, czp);
                BlockType zm = gen.GetBlockFast(x, y, z - 1, czm);

                if (IsWater(b)) {
                    if (IsAir(dn)) {
                        floatingWater++;
                        if (examples.size() < 6)
                            examples.push_back("water with air below at " +
                                std::to_string(x) + "," + std::to_string(y) + "," +
                                std::to_string(z));
                    }
                    bool sideAir = IsAir(xp) || IsAir(xm) || IsAir(zp) || IsAir(zm);
                    bool isFallLip = c.river.plunge || c.river.sill ||
                        cxp.river.plunge || cxm.river.plunge ||
                        czp.river.plunge || czm.river.plunge;
                    if (sideAir && IsAir(up) && !isFallLip) {
                        waterWall++;
                        if (examples.size() < 12)
                            examples.push_back("open water side at " +
                                std::to_string(x) + "," + std::to_string(y) + "," +
                                std::to_string(z));
                    }
                    continue;
                }

                if (IsPlantLike(b)) continue;

                if (IsAir(up) && IsAir(dn) && IsAir(xp) && IsAir(xm) &&
                    IsAir(zp) && IsAir(zm)) {
                    floatingSolid++;
                    floatKinds[(int)b]++;
                    if (examples.size() < 18)
                        examples.push_back("floating " + BlockInfo::GetName((int)b) +
                            " at " + std::to_string(x) + "," + std::to_string(y) +
                            "," + std::to_string(z));
                }
            }
        }
    }

    printf("=== floating block scan ===\n");
    printf("  area %d x %d, %ld solid/water blocks inspected\n",
        SPAN * 2, SPAN * 2, sampled);
    printf("  floating solid blocks (all 6 sides air): %ld\n", floatingSolid);
    printf("  water with air directly below:           %ld\n", floatingWater);
    printf("  water with open side and open top:       %ld\n", waterWall);

    if (!floatKinds.empty()) {
        printf("  floating block types:\n");
        for (auto& kv : floatKinds)
            printf("    %-18s %ld\n", BlockInfo::GetName(kv.first).c_str(), kv.second);
    }

    if (!examples.empty()) {
        printf("  examples:\n");
        for (const std::string& e : examples) printf("    %s\n", e.c_str());
    }

    int fails = 0;
    if (floatingSolid > 0) { printf("FAIL floating solid blocks exist\n"); fails++; }
    if (floatingWater > 0) { printf("FAIL water hangs in the air\n"); fails++; }

    printf("\n%s (%d failures)\n", fails ? "FAILED" : "ALL CHECKS PASSED", fails);
    return fails ? 1 : 0;
}
