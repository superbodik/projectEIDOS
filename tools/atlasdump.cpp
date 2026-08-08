#include "../src/World/Chunk.h"
#include "../src/Inventory/BlockInfo.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static const char* FACE_NAMES[3] = { "top", "bottom", "side" };

int main(int argc, char** argv) {
    std::string outDir = (argc > 1) ? argv[1] : "public/site/static";
    std::string pngPath = outDir + "/atlas.png";
    std::string jsonPath = outDir + "/atlas_map.json";

    SetTraceLogLevel(LOG_WARNING);

    Image atlas = Chunk::BuildAtlasImage();
    if (!ExportImage(atlas, pngPath.c_str())) {
        printf("FAILED to write %s\n", pngPath.c_str());
        return 1;
    }
    printf("atlas: %s (%dx%d)\n", pngPath.c_str(), atlas.width, atlas.height);

    const int GRID = 16;
    const float step = Chunk::TileStep();

    std::ofstream out(jsonPath, std::ios::trunc | std::ios::binary);
    if (!out) {
        printf("FAILED to write %s\n", jsonPath.c_str());
        UnloadImage(atlas);
        return 1;
    }

    out << "{\n";
    out << "  \"grid\": " << GRID << ",\n";
    out << "  \"tile\": " << (atlas.width / GRID) << ",\n";
    out << "  \"atlas\": \"atlas.png\",\n";
    out << "  \"blocks\": {\n";

    int written = 0;
    int missing = 0;

    for (int id = 1; id <= 255; id++) {
        std::string name = BlockInfo::GetName(id);
        if (name.empty() || name == "Unknown" || name.rfind("Block ", 0) == 0) continue;

        int cols[3], rows[3];
        bool ok = true;
        for (int face = 0; face < 3; face++) {
            float u = -1.0f, v = -1.0f;
            Chunk::GetTextureUV((BlockType)id, face, u, v);
            if (u < 0.0f || v < 0.0f) { ok = false; break; }
            cols[face] = (int)(u / step + 0.5f);
            rows[face] = (int)(v / step + 0.5f);
        }
        if (!ok) { missing++; continue; }

        if (written) out << ",\n";
        out << "    \"" << id << "\": {\"name\": \"" << name << "\"";
        for (int face = 0; face < 3; face++)
            out << ", \"" << FACE_NAMES[face] << "\": [" << cols[face] << ", " << rows[face] << "]";
        out << "}";
        written++;
    }

    out << "\n  }\n}\n";
    out.close();

    printf("map:   %s (%d blocks", jsonPath.c_str(), written);
    if (missing) printf(", %d skipped", missing);
    printf(")\n");

    UnloadImage(atlas);
    return written > 0 ? 0 : 1;
}
