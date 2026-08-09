from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
ROADMAP = PROJECT_ROOT / "ROADMAP.md"

ENTRY_RE = re.compile(r"^-\s+(?:(\S+)\s+)?\*\*(.+?)\*\*\s*\(([^)]*)\)\s*[:—-]?\s*(.*)$")
DATE_RE = re.compile(r"^(\d{2})\.(\d{2})\.(\d{4})(?:\s+(\d{2}):(\d{2}))?")

FIX_WORDS = ("почин", "исправ", "фикс", "баг", "убра", "устран")
CHANGE_WORDS = ("переписан", "переехал", "заменен", "заменён", "стало", "→")


def load_env() -> None:
    env_path = PROJECT_ROOT / "public" / "site" / ".env"
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


def parse_roadmap(since_date: str = "") -> list[dict]:
    if not ROADMAP.exists():
        return []

    entries: list[dict] = []
    current: dict | None = None

    for raw in ROADMAP.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = raw.strip()
        m = ENTRY_RE.match(stripped)
        if m and DATE_RE.match(m.group(3).strip()):
            if current:
                entries.append(current)
            current = {
                "title": m.group(2).strip(),
                "date": m.group(3).strip(),
                "summary": m.group(4).strip(),
                "details": [],
            }
            continue

        if current is not None:
            if stripped.startswith("#"):
                entries.append(current)
                current = None
            elif stripped.startswith("- ") and not raw.startswith("  "):
                entries.append(current)
                current = None
            elif stripped.startswith("-") and raw.startswith("  "):
                current["details"].append(stripped.lstrip("- ").strip())

    if current:
        entries.append(current)

    def key(e: dict):
        m = DATE_RE.match(e["date"])
        return (m.group(3), m.group(2), m.group(1), m.group(4) or "00", m.group(5) or "00")

    entries.sort(key=key, reverse=True)

    if since_date:
        sm = DATE_RE.match(since_date)
        if sm:
            floor = (sm.group(3), sm.group(2), sm.group(1), sm.group(4) or "00", sm.group(5) or "00")
            entries = [e for e in entries if key(e) > floor]

    return entries


def classify(entries: list[dict]) -> dict:
    added: list[str] = []
    fixed: list[str] = []
    changed: list[str] = []

    for e in entries:
        head = e["title"]
        blob = (head + " " + e["summary"]).lower()

        bucket = added
        if any(w in blob for w in FIX_WORDS):
            bucket = fixed
        elif any(w in blob for w in CHANGE_WORDS):
            bucket = changed

        line = head if not e["summary"] else f"{head} — {e['summary']}"
        bucket.append(line[:400])

        for d in e["details"][:4]:
            low = d.lower()
            target = fixed if any(w in low for w in FIX_WORDS) else bucket
            target.append(d[:400])

    def dedupe(items: list[str]) -> list[str]:
        seen = set()
        out = []
        for i in items:
            k = i[:80]
            if k in seen:
                continue
            seen.add(k)
            out.append(i)
        return out[:40]

    return {"added": dedupe(added), "fixed": dedupe(fixed), "changed": dedupe(changed)}


def post(site: str, token: str, payload: dict) -> int:
    url = f"{site.rstrip('/')}/api/admin/updates"
    data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(url, data=data, method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Authorization", f"Bearer {token}")
    # Cloudflare's bot rules 403 the default "Python-urllib/x.y" UA outright.
    req.add_header("User-Agent", "eidos-tools/1.0 (+publish_update.py)")

    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            entry = body.get("update", {})
            print(f"[publish] {resp.status} {entry.get('version')} "
                  f"'{entry.get('title')}' ({entry.get('total')} lines)")
            return 0
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        print(f"[publish] HTTP {exc.code}: {detail[:400]}")
        if exc.code == 404:
            print("[publish] ADMIN_TOKEN is unset on the server - the endpoint is disabled")
        return 1
    except urllib.error.URLError as exc:
        print(f"[publish] cannot reach {site}: {exc}")
        return 1


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    load_env()

    ap = argparse.ArgumentParser(
        description="Publish a changelog entry to the CRC EIDOS site from ROADMAP.md")
    ap.add_argument("--version", required=True)
    ap.add_argument("--title", default="")
    ap.add_argument("--summary", default="")
    ap.add_argument("--channel", default="beta", choices=["beta", "stable", "nightly"])
    ap.add_argument("--since", default="",
                    help="only roadmap entries newer than this date, e.g. 02.08.2026")
    ap.add_argument("--site", default=os.environ.get("EIDOS_SITE", "http://127.0.0.1:8001"))
    ap.add_argument("--token", default=os.environ.get("ADMIN_TOKEN", ""))
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    entries = parse_roadmap(args.since)
    if not entries:
        print("[publish] nothing found in ROADMAP.md for that range")
        return 1

    body = classify(entries)
    payload = {
        "version": args.version,
        "title": args.title or entries[0]["title"],
        "channel": args.channel,
        "summary": args.summary,
        "published_at": entries[0]["date"],
        **body,
    }

    print(f"[publish] {args.version} from {len(entries)} roadmap entries: "
          f"{len(body['added'])} added, {len(body['fixed'])} fixed, "
          f"{len(body['changed'])} changed")

    if args.dry_run:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0

    if not args.token:
        print("[publish] no ADMIN_TOKEN - set it in public/site/.env or pass --token")
        return 1

    return post(args.site, args.token, payload)


if __name__ == "__main__":
    raise SystemExit(main())
