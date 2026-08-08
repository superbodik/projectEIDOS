#include "../src/World/WorldGenerator.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <limits>

struct Stat {
    long columns = 0;
    float tMin = 1e9f, tMax = -1e9f, tSum = 0.0f;
    float hMin = 1e9f, hMax = -1e9f, hSum = 0.0f;
    int   eMin = 100000, eMax = -100000;
    long  eSum = 0;
    std::map<int, long> ground;
    std::map<int, long> canopy;
};

static std::string JsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

static void TopN(const std::map<int, long>& counts, long total, int n,
    std::vector<std::pair<int, long>>& out) {
    std::vector<std::pair<int, long>> all(counts.begin(), counts.end());
    std::sort(all.begin(), all.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    for (int i = 0; i < (int)all.size() && i < n; i++) out.push_back(all[i]);
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);

    std::map<std::string, Stat> stats;

    const int SPAN = 6000;
    const int STEP = 4;
    long total = 0;

    for (int x = -SPAN; x <= SPAN; x += STEP) {
        for (int z = -SPAN; z <= SPAN; z += STEP) {
            std::string name = gen.GetBiomeName(x, z);
            if (name.empty() || name == "River" || name == "Unknown") continue;

            ColumnInfo col = gen.GetColumnInfo(x, z);
            Stat& s = stats[name];
            s.columns++;
            total++;

            // GetTemperature() is the same 0..1 latitude index GetClimate() feeds
            // into baseC = lerp(-30, 38, t) - convert here so the wiki shows real
            // degrees instead of an unlabeled fraction nobody can interpret.
            float t01 = gen.GetTemperature(x, z);
            float t = -30.0f + 68.0f * t01;
            float h = gen.GetHumidity(x, z) * 100.0f;
            s.tMin = std::min(s.tMin, t); s.tMax = std::max(s.tMax, t); s.tSum += t;
            s.hMin = std::min(s.hMin, h); s.hMax = std::max(s.hMax, h); s.hSum += h;
            s.eMin = std::min(s.eMin, col.height);
            s.eMax = std::max(s.eMax, col.height);
            s.eSum += col.height;

            BlockType ground = gen.GetBlockFast(x, col.height, z, col);
            if (ground != BlockType::Air) s.ground[(int)ground]++;

            auto isLiquid = [](BlockType b) {
                return b == BlockType::Water || b == BlockType::WaterSource ||
                    b == BlockType::Lava || b == BlockType::LavaSource ||
                    b == BlockType::Ice;
                };

            // Vegetation should read as "what grows here", not "is this column
            // wet" - a submerged column would otherwise stack up to 8 Water
            // hits and swamp every other entry (percentages above 100%).
            for (int dy = 1; dy <= 8; dy++) {
                BlockType b = gen.GetBlockFast(x, col.height + dy, z, col);
                if (b == BlockType::Air || isLiquid(b)) continue;
                s.canopy[(int)b]++;
            }
        }
        if ((x - (-SPAN)) % 400 == 0) {
            fprintf(stderr, "\r[biomedump] x=%d/%d columns=%ld", x, SPAN, total);
        }
    }
    fprintf(stderr, "\n[biomedump] done: %ld columns across %d biomes\n",
        total, (int)stats.size());

    std::ofstream out("public/site/static/biomes.json", std::ios::binary);
    out << "{\n";
    bool firstBiome = true;
    for (auto& [name, s] : stats) {
        if (!firstBiome) out << ",\n";
        firstBiome = false;

        std::vector<std::pair<int, long>> topGround, topCanopy;
        TopN(s.ground, s.columns, 4, topGround);
        TopN(s.canopy, s.columns, 8, topCanopy);

        out << "  \"" << JsonEscape(name) << "\": {\n";
        out << "    \"share_pct\": " << (100.0 * s.columns / total) << ",\n";
        out << "    \"temperature_c\": { \"min\": " << s.tMin << ", \"max\": " << s.tMax
            << ", \"avg\": " << (s.tSum / s.columns) << " },\n";
        out << "    \"humidity_pct\": { \"min\": " << s.hMin << ", \"max\": " << s.hMax
            << ", \"avg\": " << (s.hSum / s.columns) << " },\n";
        out << "    \"elevation\": { \"min\": " << s.eMin << ", \"max\": " << s.eMax
            << ", \"avg\": " << (s.eSum / s.columns) << " },\n";

        out << "    \"ground_blocks\": [";
        for (size_t i = 0; i < topGround.size(); i++) {
            if (i) out << ", ";
            out << "{\"name\": \"" << JsonEscape(BlockInfo::GetName(topGround[i].first))
                << "\", \"pct\": " << (100.0 * topGround[i].second / s.columns) << "}";
        }
        out << "],\n";

        out << "    \"vegetation\": [";
        for (size_t i = 0; i < topCanopy.size(); i++) {
            if (i) out << ", ";
            out << "{\"name\": \"" << JsonEscape(BlockInfo::GetName(topCanopy[i].first))
                << "\", \"pct\": " << (100.0 * topCanopy[i].second / s.columns) << "}";
        }
        out << "]\n";
        out << "  }";
    }
    out << "\n}\n";
    out.close();

    printf("[biomedump] wrote public/site/static/biomes.json (%d biomes, %ld columns)\n",
        (int)stats.size(), total);
    fflush(stdout);
    std::_Exit(0);
}
