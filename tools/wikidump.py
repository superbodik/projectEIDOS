"""Generate public/site/static/wiki.json from the engine source tree.

Run this whenever engine data the wiki describes changes (new blocks,
ores, foods, quests) and before deploying. The site itself never parses
src/ at request time - wiki_data.build() reads this file instead, so the
deployed container only needs public/site/, not the whole repository.

Usage: python tools/wikidump.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

SITE_DIR = Path(__file__).resolve().parent.parent / "public" / "site"
sys.path.insert(0, str(SITE_DIR))

import wiki_data  # noqa: E402


def main() -> int:
    data = wiki_data.build_from_source()
    out_path = wiki_data.WIKI_SNAPSHOT_PATH
    out_path.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")

    print(f"[wikidump] wrote {out_path}")
    print(f"[wikidump] {len(data['blocks'])} blocks, {len(data['block_details'])} detailed, "
          f"{len(data['block_groups'])} groups")
    print(f"[wikidump] {len(data['foods'])} foods, {len(data['quests'])} quests, "
          f"{len(data['ores'])} ore tables, {len(data['biomes'])} biomes")

    with_ore = sum(1 for b in data["block_details"] if b["ore_vein"])
    with_forage = sum(1 for b in data["block_details"] if b["forage"])
    with_biomes = sum(1 for b in data["block_details"] if b["found_in_biomes"])
    print(f"[wikidump] cross-references: {with_ore} blocks with ore-vein info, "
          f"{with_forage} with forage drops, {with_biomes} tied to specific biomes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
