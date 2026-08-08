#pragma once
#include <vector>

namespace Food {

    enum Nutrient { Grain = 0, Vegetables = 1, Fruit = 2, Protein = 3, None = -1 };

    struct Def {
        int         id;
        const char* name;
        float       satiety;
        float       hydration;
        int         nutrient;
        float       amount;
        float       poison;
        const char* hint;
    };

    const std::vector<Def>& All();
    const Def* Get(int id);
    bool IsEdible(int id);
    const char* NutrientName(int nutrient);

}
