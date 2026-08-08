#pragma once
#include <string>
#include <vector>

namespace Creative {

    struct Entry {
        int         id;
        std::string name;
    };

    struct Group {
        std::string        name;
        std::vector<Entry> entries;
    };

    const std::vector<Group>& Catalog();
    int TotalEntries();
    int MissingFromCatalog();

}
