from __future__ import annotations

import json
import re
import threading
from pathlib import Path

SITE_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SITE_DIR.parent.parent
SRC = PROJECT_ROOT / "src"
BIOME_DATA_PATH = SITE_DIR / "static" / "biomes.json"
ATLAS_MAP_PATH = SITE_DIR / "static" / "atlas_map.json"
WIKI_SNAPSHOT_PATH = SITE_DIR / "static" / "wiki.json"

_lock = threading.Lock()
_cache: dict = {}
_cache_stamp: float = -1.0

WATCHED = [
    SRC / "Inventory" / "BlockInfo.cpp",
    SRC / "Inventory" / "FoodSystem.cpp",
    SRC / "Progression" / "QuestSystem.cpp",
    SRC / "World" / "WorldGenerator.cpp",
    SRC / "World" / "BlockType.h",
    BIOME_DATA_PATH,
    ATLAS_MAP_PATH,
    WIKI_SNAPSHOT_PATH,
]


def _read(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def _stamp() -> float:
    total = 0.0
    for p in WATCHED:
        if p.exists():
            total += p.stat().st_mtime
    return total


def block_names() -> dict[int, str]:
    text = _read(SRC / "Inventory" / "BlockInfo.cpp")
    out: dict[int, str] = {}
    for m in re.finditer(r'case\s+(\d+):\s*return\s+"([^"]+)"', text):
        out[int(m.group(1))] = m.group(2)
    return out


def enum_name_to_id() -> dict[str, int]:
    """C++ enum identifier -> numeric id, straight from BlockType.h.

    Needed because BlockInfo::GetName's display strings ("Copper Ore
    Pebble") don't reduce back to their enum identifier ("CopperPebble")
    by any simple rule - GetName adds descriptive words the enum doesn't
    have. ore_tables() and FORAGE_TABLE both key on the enum identifier
    (that's what WorldGenerator.cpp and Survival.cpp actually write), so
    resolving through this map is the only reliable bridge to a block id.
    """
    text = _read(SRC / "World" / "BlockType.h")
    out: dict[str, int] = {}
    for m in re.finditer(r"(\w+)\s*=\s*(\d+)", text):
        out[m.group(1)] = int(m.group(2))
    return out


def foods() -> list[dict]:
    text = _read(SRC / "Inventory" / "FoodSystem.cpp")
    pattern = re.compile(
        r"\{\s*\(int\)BlockType::(\w+)\s*,\s*"
        r'"([^"]*)"\s*,\s*'
        r"([-\d.]+)f\s*,\s*([-\d.]+)f\s*,\s*"
        r"(\w+)\s*,\s*([-\d.]+)f\s*,\s*([-\d.]+)f\s*,\s*"
        r'"([^"]*)"\s*\}',
        re.S,
    )
    out = []
    for m in pattern.finditer(text):
        out.append({
            "block": m.group(1),
            "name": m.group(2),
            "satiety": float(m.group(3)),
            "hydration": float(m.group(4)),
            "nutrient": m.group(5),
            "amount": float(m.group(6)),
            "poison": float(m.group(7)),
            "hint": m.group(8),
        })
    return out


def quests() -> list[dict]:
    text = _read(SRC / "Progression" / "QuestSystem.cpp")
    start = text.find("stone.quests = {")
    if start < 0:
        return []
    end = text.find("};", start)
    body = text[start:end]

    pattern = re.compile(
        r'(collect|anyOf|distinct|biome|depth|height)\(\s*'
        r'"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*,\s*([^\n]*)',
        re.S,
    )
    kinds = {
        "collect": "Collect",
        "anyOf": "Collect any",
        "distinct": "Collect distinct",
        "biome": "Reach biome",
        "depth": "Reach depth",
        "height": "Reach altitude",
    }

    out = []
    for m in pattern.finditer(body):
        tail = m.group(5).strip().rstrip("),").strip()
        out.append({
            "kind": kinds.get(m.group(1), m.group(1)),
            "id": m.group(2),
            "title": m.group(3),
            "desc": m.group(4).replace('\\"', '"'),
            "target": tail,
        })
    return out


def era_teasers() -> list[dict]:
    text = _read(SRC / "Progression" / "QuestSystem.cpp")
    out = []
    for m in re.finditer(
        r'teaser\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*\{([^}]*)\}', text, re.S
    ):
        titles = re.findall(r'"([^"]+)"', m.group(3))
        out.append({"name": m.group(1), "version": m.group(2), "quests": titles})
    return out


def rock_suites() -> list[dict]:
    text = _read(SRC / "World" / "WorldGenerator.cpp")

    beds: dict[str, list[dict]] = {}
    for m in re.finditer(r"const Bed (BEDS_\w+)\[\]\s*=\s*\{(.*?)\};", text, re.S):
        rows = []
        for bm in re.finditer(r"\{\s*(INT_MIN|-?\d+)\s*,\s*BlockType::(\w+)\s*\}", m.group(2)):
            top = m.group(0) and bm.group(1)
            rows.append({
                "top": None if top == "INT_MIN" else int(top),
                "rock": bm.group(2),
            })
        beds[m.group(1)] = rows

    suites = []
    sm = re.search(r"const Suite SUITES\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if sm:
        for m in re.finditer(
            r"\{\s*(BEDS_\w+)\s*,\s*\d+\s*,\s*BlockType::(\w+)\s*,\s*\"([^\"]+)\"\s*\}",
            sm.group(1),
        ):
            suites.append({
                "name": m.group(3),
                "vein": m.group(2),
                "beds": beds.get(m.group(1), []),
            })
    return suites


def ore_tables() -> list[dict]:
    text = _read(SRC / "World" / "WorldGenerator.cpp")
    labels = {
        "ORE_CARBONATE": "Carbonate rock (limestone, dolomite, chalk, marble)",
        "ORE_SEDIMENTARY": "Sedimentary rock (shale, sandstone, chert, claystone)",
        "ORE_IGNEOUS": "Volcanic rock (basalt, andesite, gabbro, dacite)",
        "ORE_GRANITIC": "Granitic rock (granite, diorite, rhyolite)",
        "ORE_METAMORPHIC": "Metamorphic rock (schist, gneiss, quartzite, slate)",
    }
    out = []
    for key, label in labels.items():
        m = re.search(r"const OreDef %s\[\]\s*=\s*\{(.*?)\};" % key, text, re.S)
        if not m:
            continue
        ores = []
        for om in re.finditer(
            r"\{\s*BlockType::(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d.]+)f\s*\}", m.group(1)
        ):
            ores.append({
                "ore": om.group(1),
                "y_min": int(om.group(2)),
                "y_max": int(om.group(3)),
                "rate": float(om.group(4)),
            })
        out.append({"host": label, "ores": ores})
    return out


def biomes() -> list[str]:
    text = _read(SRC / "World" / "WorldGenerator.cpp")
    m = re.search(r"std::string WorldGenerator::GetBiomeName.*?\n\}", text, re.S)
    if not m:
        return []
    found = re.findall(r'return\s+"([^"]+)"', m.group(0))
    seen = set()
    out = []
    for name in found:
        if name in ("Unknown", "") or name in seen:
            continue
        seen.add(name)
        out.append(name)
    return out


def biome_details() -> dict:
    """Empirical per-biome climate/ground/vegetation stats.

    Generated by tools/biomedump.cpp, which samples the real WorldGenerator
    rather than parsing its decision tree - the tree mixes relief, variant
    hashes and blending too tightly for a regex to describe honestly.
    """
    if not BIOME_DATA_PATH.exists():
        return {}
    try:
        return json.loads(BIOME_DATA_PATH.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}


def atlas_map() -> dict:
    if not ATLAS_MAP_PATH.exists():
        return {"grid": 16, "tile": 16, "blocks": {}}
    try:
        return json.loads(ATLAS_MAP_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {"grid": 16, "tile": 16, "blocks": {}}


BLOCK_GROUPS = [
    ("Rock", range(16, 46)),
    ("Ore veins", list(range(50, 67)) + list(range(180, 186))),
    ("Ground", range(5, 16)),
    ("Wood and leaves", range(100, 110)),
    ("Plants", range(110, 130)),
    ("Pebbles", list(range(130, 150)) + list(range(190, 197))),
    ("Food and forage", range(150, 160)),
    ("Liquids and ice", [1, 2, 3, 4, 120, 121, 122]),
]

# Hand-curated, not auto-extracted: EidosEngine::GrantForage (Survival.cpp)
# branches on many conditions with per-branch roll thresholds that don't
# reduce to a flat table a regex could read safely - the same reasoning
# that keeps biome selection off the parser and on tools/biomedump.cpp.
# Keep this in sync by hand whenever GrantForage changes.
FORAGE_TABLE = {
    "OakLeaves": [
        {"drop": "Acorn", "chance_pct": 10.0},
        {"drop": "BirdEgg", "chance_pct": 1.5},
    ],
    "OakLog": [{"drop": "Grubs", "chance_pct": 14.0}],
    "BirchLog": [{"drop": "Grubs", "chance_pct": 14.0}],
    "SpruceLog": [{"drop": "Grubs", "chance_pct": 14.0}],
    "JungleLog": [{"drop": "Grubs", "chance_pct": 14.0}],
    "AcaciaLog": [{"drop": "Grubs", "chance_pct": 14.0}],
    "TallGrass": [{"drop": "PlantFibre", "chance_pct": 45.0}],
    "Fern": [{"drop": "PlantFibre", "chance_pct": 45.0}],
    "Clay": [{"drop": "ClayLump", "amount": "3-5", "chance_pct": 100.0}],
    "CopperPebble": [{"drop": "CopperNugget", "amount": "1-3", "chance_pct": 100.0}],
    "TinPebble": [{"drop": "TinNugget", "amount": "1-3", "chance_pct": 100.0}],
    "SilverPebble": [{"drop": "SilverNugget", "amount": "1-3", "chance_pct": 100.0}],
    "GoldPebble": [{"drop": "GoldNugget", "amount": "1-3", "chance_pct": 100.0}],
    "BerryBushRipe": [{"drop": "Berries", "amount": "1-4", "chance_pct": 100.0}],
}


def block_details() -> list[dict]:
    """One entry per block: category, texture, which biomes it turns up
    in, where its ore vein sits underground, and what breaking it can
    forage. Cross-referenced from data already extracted elsewhere
    (atlas_map.json, biome_details(), ore_tables()) rather than adding
    a second, divergent way of reading the same facts."""
    names = block_names()
    enum_ids = enum_name_to_id()
    amap = atlas_map()
    atlas_blocks = amap.get("blocks", {})
    tile = amap.get("tile", 16)

    category_by_id: dict[int, str] = {}
    for label, ids in BLOCK_GROUPS:
        for bid in ids:
            category_by_id.setdefault(bid, label)

    found_in: dict[str, set[str]] = {}
    for biome_name, info in biome_details().items():
        for entry in info.get("ground_blocks", []) + info.get("vegetation", []):
            found_in.setdefault(entry["name"], set()).add(biome_name)

    ore_by_id: dict[int, dict] = {}
    for host in ore_tables():
        for ore in host["ores"]:
            bid = enum_ids.get(ore["ore"])
            if bid is None:
                continue
            ore_by_id[bid] = {
                "host_rock": host["host"],
                "y_min": ore["y_min"],
                "y_max": ore["y_max"],
                "rate": ore["rate"],
            }

    forage_by_id: dict[int, list] = {}
    for enum_name, drops in FORAGE_TABLE.items():
        bid = enum_ids.get(enum_name)
        if bid is not None:
            forage_by_id[bid] = drops

    out = []
    for bid, name in sorted(names.items()):
        entry = atlas_blocks.get(str(bid))
        tex = None
        if entry:
            col, row = entry.get("side", [0, 0])
            tex = {"x": col * tile, "y": row * tile, "sheet": amap.get("grid", 16) * tile}

        out.append({
            "id": bid,
            "name": name,
            "category": category_by_id.get(bid, "Other"),
            "texture": tex,
            "found_in_biomes": sorted(found_in.get(name, [])),
            "ore_vein": ore_by_id.get(bid),
            "forage": forage_by_id.get(bid, []),
        })
    return out


def block_groups() -> list[dict]:
    """Blocks bucketed for the tile grid the wiki page renders - built
    from block_details() so the grouping, texture position and the rich
    per-block facts all come from one pass instead of two."""
    by_category: dict[str, list[dict]] = {}
    for b in block_details():
        by_category.setdefault(b["category"], []).append(b)

    out = []
    for label, _ids in BLOCK_GROUPS:
        items = by_category.get(label, [])
        if items:
            out.append({"label": label, "blocks": items})
    return out


CONTROLS = [
    ("WASD", "Move", "Рух"),
    ("Space", "Jump", "Стрибок"),
    ("Shift", "Sprint (needs satiety)", "Біг (потребує ситості)"),
    ("Mouse", "Look", "Огляд"),
    ("LMB", "Break block", "Зламати блок"),
    ("RMB", "Place block / eat food in hand", "Поставити блок / з'їсти їжу в руці"),
    ("1-9", "Hotbar slot", "Слот хотбару"),
    ("Q", "Drop one item", "Викинути предмет"),
    ("E", "Inventory (Items / Nutrition / Environment)", "Інвентар (Предмети / Їжа / Середовище)"),
    ("L", "Quest tree", "Дерево квестів"),
    ("F1", "Hide HUD", "Сховати HUD"),
    ("F3", "Debug overlay", "Дебаг-оверлей"),
    ("F11", "Fullscreen", "Повний екран"),
    ("~", "Console", "Консоль"),
    ("Esc", "Pause menu", "Меню паузи"),
]

COMMANDS = [
    ("geology", "Read the geology under your feet: province, surface rock, strata, veins",
     "Прочитати геологію під ногами: провінція, порода, пласти, жили"),
    ("locate biome <name>", "Find the nearest biome; 'locate biome list' shows all names",
     "Знайти найближчий біом; 'locate biome list' покаже всі назви"),
    ("wind <0-30> | wind auto", "Lock wind speed or return it to the biome",
     "Зафіксувати силу вітру або повернути автоматичну"),
    ("time set <day|night|noon|midnight|sunrise|sunset>", "Set the time of day",
     "Встановити час доби"),
    ("give @s <block> <count>", "Give yourself blocks", "Видати собі блоки"),
    ("tp <x> <y> <z>", "Teleport", "Телепорт"),
    ("gm <0|1|2>", "Game mode: survival, creative, spectator",
     "Режим гри: виживання, креатив, спостерігач"),
    ("fps_max <n>", "Frame cap, 0 for unlimited", "Ліміт кадрів, 0 — без ліміту"),
    ("save", "Save the world now", "Зберегти світ"),
]


def build_from_source() -> dict:
    """Extracts everything by reading engine source directly. This is
    what tools/wikidump.py runs to produce the static snapshot - it needs
    the full src/ tree, which is why the deployed site does not call this
    at request time (see build())."""
    return {
        "blocks": block_names(),
        "block_details": block_details(),
        "block_groups": block_groups(),
        "foods": foods(),
        "quests": quests(),
        "eras": era_teasers(),
        "suites": rock_suites(),
        "ores": ore_tables(),
        "biomes": biomes(),
        "biome_details": biome_details(),
        "controls": CONTROLS,
        "commands": COMMANDS,
    }


def build() -> dict:
    """Prefers the static snapshot (public/site/static/wiki.json, made by
    tools/wikidump.py) so the running site never needs the engine's C++
    source tree - only falls back to live source-parsing when no
    snapshot has been generated yet, e.g. right after cloning for local
    development."""
    if WIKI_SNAPSHOT_PATH.exists():
        try:
            return json.loads(WIKI_SNAPSHOT_PATH.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            pass
    return build_from_source()


def get() -> dict:
    global _cache, _cache_stamp
    with _lock:
        stamp = _stamp()
        if stamp != _cache_stamp or not _cache:
            _cache = build()
            _cache_stamp = stamp
        return _cache
