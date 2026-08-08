#include "WorldRules.h"
#include <sstream>

WorldRules WorldRules::Preset(Difficulty d) {
    WorldRules r;
    r.difficulty = d;

    switch (d) {
    case Difficulty::Casual:
        r.dayLength = 720.0f;
        r.hungerRate = 0.5f;
        r.thirstRate = 0.5f;
        r.nutrientDecay = 0.4f;
        r.damageScale = 0.5f;
        r.regenFloor = 0.60f;
        r.regenSatietyGate = 0.35f;
        r.regenThirstGate = 0.30f;
        r.comfortLow = 5.0f;
        r.comfortHigh = 34.0f;
        r.coldThreshold = 28.0f;
        r.heatThreshold = 42.0f;
        r.deathPenalty = 0;
        r.oreDensity = 1.5f;
        break;

    case Difficulty::Hardcore:
        r.dayLength = 360.0f;
        r.hungerRate = 1.5f;
        r.thirstRate = 1.6f;
        r.nutrientDecay = 1.4f;
        r.damageScale = 2.0f;
        r.regenFloor = 0.15f;
        r.regenSatietyGate = 0.70f;
        r.regenThirstGate = 0.55f;
        r.comfortLow = 15.0f;
        r.comfortHigh = 24.0f;
        r.coldThreshold = 34.5f;
        r.heatThreshold = 38.5f;
        r.deathPenalty = 2;
        r.oreDensity = 0.7f;
        break;

    case Difficulty::Survival:
    case Difficulty::Custom:
    default:
        break;
    }

    return r;
}

const char* WorldRules::Name(Difficulty d) {
    switch (d) {
    case Difficulty::Casual:   return "CASUAL";
    case Difficulty::Survival: return "SURVIVAL";
    case Difficulty::Hardcore: return "HARDCORE";
    default:                    return "CUSTOM";
    }
}

const char* WorldRules::Blurb(Difficulty d) {
    switch (d) {
    case Difficulty::Casual:
        return "Build and explore. Hunger and cold are gentle, death costs nothing.";
    case Difficulty::Survival:
        return "The intended game. Death drops your items. Diet caps your healing.";
    case Difficulty::Hardcore:
        return "Preparation or death. Narrow comfort band, healing capped at 15% "
               "without a full diet, and dying costs max health for a day.";
    default:
        return "Your own rules.";
    }
}

std::string WorldRules::Serialize() const {
    std::ostringstream o;
    o << "rules 3 "
        << (int)difficulty << ' '
        << dayLength << ' ' << hungerRate << ' ' << thirstRate << ' '
        << nutrientDecay << ' ' << damageScale << ' '
        << regenFloor << ' ' << regenSatietyGate << ' ' << regenThirstGate << ' '
        << comfortLow << ' ' << comfortHigh << ' '
        << coldThreshold << ' ' << heatThreshold << ' '
        << deathPenalty << ' ' << (oneLife ? 1 : 0) << ' '
        << oreDensity << ' ' << treeDensity << ' '
        << (questsEnabled ? 1 : 0) << ' '
        << caveDensity << ' ' << latitudeScale;
    return o.str();
}

bool WorldRules::Deserialize(const std::string& line) {
    std::istringstream in(line);
    std::string tag;
    int version = 0;
    if (!(in >> tag >> version) || tag != "rules") return false;

    int diff = 1, penalty = 1, one = 0, quests = 1;
    WorldRules r;

    if (!(in >> diff
        >> r.dayLength >> r.hungerRate >> r.thirstRate
        >> r.nutrientDecay >> r.damageScale
        >> r.regenFloor >> r.regenSatietyGate >> r.regenThirstGate
        >> r.comfortLow >> r.comfortHigh
        >> r.coldThreshold >> r.heatThreshold
        >> penalty >> one >> r.oreDensity >> r.treeDensity >> quests))
        return false;

    if (version >= 3) {
        if (!(in >> r.caveDensity >> r.latitudeScale)) {
            r.caveDensity = 1.0f;
            r.latitudeScale = 7000.0f;
        }
    }

    if (diff < 0 || diff > 3) diff = 1;
    r.difficulty = (Difficulty)diff;
    r.deathPenalty = (penalty < 0 || penalty > 2) ? 1 : penalty;
    r.oneLife = (one != 0);
    r.questsEnabled = (quests != 0);

    *this = r;
    return true;
}
