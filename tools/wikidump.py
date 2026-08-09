"""Generate public/site/static/wiki.json from the engine source tree,
and optionally push it straight to a running api.py.

Run this whenever engine data the wiki describes changes (new blocks,
ores, foods, quests) and before deploying. The site itself never parses
src/ at request time - wiki_data.build() reads this file instead, so the
deployed container only needs public/site/, not the whole repository.

Usage:
    python tools/wikidump.py
    python tools/wikidump.py --push --site https://api.eidos.pp.ua --token <ADMIN_TOKEN>
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
SITE_DIR = PROJECT_ROOT / "public" / "site"
sys.path.insert(0, str(SITE_DIR))

import wiki_data  # noqa: E402


def load_env() -> None:
    env_path = SITE_DIR / ".env"
    if not env_path.exists():
        return
    for line in env_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, _, v = line.partition("=")
        k, v = k.strip(), v.strip()
        if k and k not in os.environ:
            os.environ[k] = v


def push(site: str, token: str, data: dict) -> int:
    url = f"{site.rstrip('/')}/api/admin/wiki"
    body = json.dumps(data, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(url, data=body, method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Authorization", f"Bearer {token}")

    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            reply = json.loads(resp.read().decode("utf-8"))
            print(f"[wikidump] pushed to {url}: {reply.get('blocks')} blocks, "
                  f"{reply.get('biomes')} biomes")
            return 0
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        print(f"[wikidump] HTTP {exc.code}: {detail[:400]}")
        return 1
    except urllib.error.URLError as exc:
        print(f"[wikidump] cannot reach {site}: {exc}")
        return 1


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    load_env()

    ap = argparse.ArgumentParser(description="Generate (and optionally push) the wiki data snapshot")
    ap.add_argument("--push", action="store_true", help="also POST the snapshot to a running api.py")
    ap.add_argument("--site", default=os.environ.get("EIDOS_SITE", "http://127.0.0.1:2976"))
    ap.add_argument("--token", default=os.environ.get("ADMIN_TOKEN", ""))
    args = ap.parse_args()

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

    if args.push:
        if not args.token:
            print("[wikidump] --push needs an admin token: set ADMIN_TOKEN in "
                  "public/site/.env or pass --token")
            return 1
        return push(args.site, args.token, data)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
