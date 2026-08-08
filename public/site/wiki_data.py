from __future__ import annotations

import json
import re
import threading
from pathlib import Path

SITE_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SITE_DIR.parent.parent
SRC = PROJECT_ROOT / "src"
BIOME_DATA_PATH = SITE_DIR / "static" / "biomes.json"

_lock = threading.Lock()
_cache: dict = {}
_cache_stamp: float = -1.0

WATCHED = [
    SRC / "Inventory" / "BlockInfo.cpp",
    SRC / "Inventory" / "FoodSystem.cpp",
    SRC / "Progression" / "QuestSystem.cpp",
    SRC / "World" / "WorldGenerator.cpp",
    PROJECT_ROOT / "ROADMAP.md",
    BIOME_DATA_PATH,
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


_MONTHS = re.compile(r"^(\d{2})\.(\d{2})\.(\d{4})(?:\s+(\d{2}):(\d{2}))?")


def changelog(limit: int = 200) -> list[dict]:
    text = _read(PROJECT_ROOT / "ROADMAP.md")
    if not text:
        return []

    entry_re = re.compile(
        r"^-\s+(?:(\S+)\s+)?\*\*(.+?)\*\*\s*\(([^)]*)\)\s*[:—-]?\s*(.*)$"
    )

    lines = text.splitlines()
    entries: list[dict] = []
    current: dict | None = None

    for line in lines:
        stripped = line.strip()
        m = entry_re.match(stripped)
        if m and _MONTHS.match(m.group(3).strip()):
            if current:
                entries.append(current)
            current = {
                "icon": m.group(1) or "",
                "title": m.group(2).strip(),
                "date": m.group(3).strip(),
                "summary": m.group(4).strip(),
                "details": [],
            }
            continue

        if current is not None:
            if stripped.startswith("- ") and not line.startswith("  "):
                entries.append(current)
                current = None
                continue
            if stripped.startswith("-") and line.startswith("  "):
                current["details"].append(stripped.lstrip("- ").strip())
            elif stripped.startswith("#"):
                entries.append(current)
                current = None

    if current:
        entries.append(current)

    def sort_key(e: dict):
        m = _MONTHS.match(e["date"])
        if not m:
            return ("0000", "00", "00", "00", "00")
        return (m.group(3), m.group(2), m.group(1), m.group(4) or "00", m.group(5) or "00")

    entries.sort(key=sort_key, reverse=True)
    return entries[:limit]


def roadmap_version() -> str:
    text = _read(PROJECT_ROOT / "ROADMAP.md")
    m = re.search(r"v(\d+\.\d+(?:\.\d+)?)", text[:400])
    return f"v{m.group(1)}" if m else "unknown"


def roadmap_updated() -> str:
    text = _read(PROJECT_ROOT / "ROADMAP.md")
    m = re.search(r"\*Обновлено:\s*([^*]+)\*", text)
    return m.group(1).strip() if m else ""


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


def build() -> dict:
    return {
        "version": roadmap_version(),
        "updated": roadmap_updated(),
        "blocks": block_names(),
        "foods": foods(),
        "quests": quests(),
        "eras": era_teasers(),
        "suites": rock_suites(),
        "ores": ore_tables(),
        "biomes": biomes(),
        "biome_details": biome_details(),
        "changelog": changelog(),
        "controls": CONTROLS,
        "commands": COMMANDS,
    }


def get() -> dict:
    global _cache, _cache_stamp
    with _lock:
        stamp = _stamp()
        if stamp != _cache_stamp or not _cache:
            _cache = build()
            _cache_stamp = stamp
        return _cache
