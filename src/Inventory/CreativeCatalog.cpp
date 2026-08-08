#include "CreativeCatalog.h"
#include "BlockInfo.h"
#include "../World/BlockType.h"

namespace {

    struct Range { const char* name; int lo; int hi; };

    const Range GROUPS[] = {
        { "Liquids",        1,  4   },
        { "Ground",         5,  15  },
        { "Stone",          16, 19  },
        { "Sedimentary",    20, 29  },
        { "Igneous",        30, 39  },
        { "Metamorphic",    40, 49  },
        { "Ore",            50, 69  },
        { "Wood",          100, 109 },
        { "Plants",        110, 119 },
        { "Ice and snow",  120, 122 },
        { "Crafted",       123, 129 },
        { "Pebbles",       130, 149 },
        { "Food",          150, 159 },
    };

    const int GROUP_COUNT = (int)(sizeof(GROUPS) / sizeof(GROUPS[0]));
    const int MAX_ID = 255;

    bool HasRealName(int id, std::string& out) {
        out = BlockInfo::GetName(id);
        if (out.empty()) return false;
        if (out == "Unknown") return false;
        if (out.rfind("Block ", 0) == 0) return false;
        return true;
    }

    std::vector<Creative::Group> Build(int& missing) {
        std::vector<Creative::Group> catalog;
        catalog.reserve(GROUP_COUNT + 1);

        bool claimed[MAX_ID + 1] = { false };

        for (int g = 0; g < GROUP_COUNT; g++) {
            Creative::Group group;
            group.name = GROUPS[g].name;

            for (int id = GROUPS[g].lo; id <= GROUPS[g].hi && id <= MAX_ID; id++) {
                std::string name;
                if (!HasRealName(id, name)) continue;
                group.entries.push_back({ id, name });
                claimed[id] = true;
            }

            if (!group.entries.empty()) catalog.push_back(std::move(group));
        }

        Creative::Group other;
        other.name = "Other";
        for (int id = 1; id <= MAX_ID; id++) {
            if (claimed[id]) continue;
            std::string name;
            if (!HasRealName(id, name)) continue;
            other.entries.push_back({ id, name });
        }

        missing = (int)other.entries.size();
        if (!other.entries.empty()) catalog.push_back(std::move(other));

        return catalog;
    }

    int g_missing = 0;

}

namespace Creative {

    const std::vector<Group>& Catalog() {
        static std::vector<Group> catalog = Build(g_missing);
        return catalog;
    }

    int TotalEntries() {
        int n = 0;
        for (const Group& g : Catalog()) n += (int)g.entries.size();
        return n;
    }

    int MissingFromCatalog() {
        Catalog();
        return g_missing;
    }

}
