#pragma once
#include <sol/sol.hpp>
#include "../Logic/SeedProfile.h"
#include "../Logic/EidosTypes.h"
#include "../Logic/USMMath.h"

class LuaBridge {
public:
    static void Bind(sol::state& lua) {
        lua.new_enum("Stat",
            "Might", StatType::Might,
            "Cunning", StatType::Cunning,
            "Endurance", StatType::Endurance,
            "Focus", StatType::Focus,
            "Social", StatType::Social,
            "Intellect", StatType::Intellect,
            "Luck", StatType::Luck,
            "Mastery", StatType::Mastery
        );

        lua.new_usertype<SeedProfile>("SeedProfile",
            "get", &SeedProfile::get,
            "set", &SeedProfile::set
        );

        lua.new_usertype<USMMath>("USMMath",
            "Convert", &USMMath::Convert
        );
    }
};
