#include "../src/World/Chunk.h"
#include "../src/World/WorldGenerator.h"
#include <cstdio>
#include <chrono>
#include <vector>
#include <memory>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    WorldGenerator gen(1337);

    const int R = 5;
    const int SIDE = R * 2 + 1;
    const int N = SIDE * SIDE;

    printf("=== the full cost of getting one chunk on screen ===\n");
    printf("    %d chunks (%dx%d), CPU stages only, single thread\n\n", N, SIDE, SIDE);

    auto& chunks = *(new std::vector<std::shared_ptr<Chunk>>());
    chunks.reserve(N);
    for (int cx = -R; cx <= R; cx++)
        for (int cz = -R; cz <= R; cz++)
            chunks.push_back(std::make_shared<Chunk>(cx, cz));

    auto At = [&](int cx, int cz) -> Chunk* {
        if (cx < -R || cx > R || cz < -R || cz > R) return nullptr;
        return chunks[(cx + R) * SIDE + (cz + R)].get();
    };

    auto t0 = Clock::now();
    for (auto& c : chunks) c->GenerateTerrain(gen);
    auto t1 = Clock::now();
    double tGen = ms(t0, t1);

    double tLight = 0, tMesh = 0;
    for (auto& c : chunks) {
        Chunk* nb[9];
        for (int dz = -1; dz <= 1; dz++)
            for (int dx = -1; dx <= 1; dx++)
                nb[(dx + 1) + (dz + 1) * 3] = At(c->chunkX + dx, c->chunkZ + dz);

        auto a = Clock::now();
        c->CalculateBasicSunlight(nb);
        auto b = Clock::now();
        tLight += ms(a, b);

        c->BuildMeshCPU(gen, nb);
        auto d = Clock::now();
        tMesh += ms(b, d);
    }

    double total = tGen + tLight + tMesh;
    struct Row { const char* name; double t; };
    Row rows[] = {
        { "GenerateTerrain ", tGen   },
        { "sunlight        ", tLight },
        { "BuildMeshCPU    ", tMesh  },
    };
    for (const Row& r : rows)
        printf("  %s %8.1f ms  %5.1f%%   %6.2f ms/chunk\n",
            r.name, r.t, 100.0 * r.t / total, r.t / N);

    printf("  %-16s %8.1f ms          %6.2f ms/chunk\n", "TOTAL", total, total / N);

    int r17 = 35 * 35;
    printf("\n  a 17-chunk radius is %d chunks\n", r17);
    printf("  one thread would need %.1f s\n", (total / N) * r17 / 1000.0);
    printf("  across 11 workers, ideally    %.1f s\n", (total / N) * r17 / 1000.0 / 11.0);

    printf("\n  slowest single stage per chunk decides the pipeline rate:\n");
    printf("    terrain %.2f ms  |  light+mesh %.2f ms\n",
        tGen / N, (tLight + tMesh) / N);
    printf("    with 4 terrain / 7 mesh workers:\n");
    printf("      terrain stage %.2f ms/chunk effective\n", (tGen / N) / 4.0);
    printf("      mesh stage    %.2f ms/chunk effective\n", ((tLight + tMesh) / N) / 7.0);

    fflush(stdout);
    std::_Exit(0);
}
