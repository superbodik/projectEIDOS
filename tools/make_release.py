from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
RELEASES_DIR = PROJECT_ROOT / "public" / "site" / "releases"
INDEX_PATH = RELEASES_DIR / "index.json"

BUILD_CANDIDATES = [
    PROJECT_ROOT / "build" / "bin" / "Release",
    PROJECT_ROOT / "build" / "bin" / "Debug",
]

PAYLOAD_DIRS = ["assets"]
SKIP_SUFFIXES = {".pdb", ".ilk", ".exp", ".lib", ".log"}


def read_version() -> str:
    roadmap = PROJECT_ROOT / "ROADMAP.md"
    if not roadmap.exists():
        return "v0.0"
    head = roadmap.read_text(encoding="utf-8", errors="replace")[:400]
    m = re.search(r"v(\d+\.\d+(?:\.\d+)?)", head)
    return f"v{m.group(1)}" if m else "v0.0"


def read_notes(limit: int = 6) -> str:
    roadmap = PROJECT_ROOT / "ROADMAP.md"
    if not roadmap.exists():
        return ""
    entry_re = re.compile(r"^-\s+(?:\S+\s+)?\*\*(.+?)\*\*\s*\(([^)]*)\)")
    date_re = re.compile(r"^\d{2}\.\d{2}\.\d{4}")
    found = []
    for line in roadmap.read_text(encoding="utf-8", errors="replace").splitlines():
        m = entry_re.match(line.strip())
        if not m:
            continue
        date = m.group(2).strip()
        if not date_re.match(date):
            continue
        found.append((date, m.group(1).strip()))
    found.sort(key=lambda p: (p[0][6:10], p[0][3:5], p[0][0:2], p[0][11:]), reverse=True)
    return " · ".join(title for _, title in found[:limit])


def find_build() -> Path | None:
    for d in BUILD_CANDIDATES:
        if (d / "EidosApp.exe").is_file():
            return d
    return None


def sha256_of(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def collect_files(build_dir: Path) -> list[tuple[Path, str]]:
    files: list[tuple[Path, str]] = []

    for item in build_dir.iterdir():
        if not item.is_file():
            continue
        if item.suffix.lower() in SKIP_SUFFIXES:
            continue
        files.append((item, item.name))

    for rel in PAYLOAD_DIRS:
        root = PROJECT_ROOT / rel
        if not root.is_dir():
            print(f"[make_release] WARNING: missing {rel}/ - the build may not start")
            continue
        for path in root.rglob("*"):
            if path.is_file():
                files.append((path, str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")))

    return files


def build_zip(version: str, channel: str, platform: str) -> Path:
    build_dir = find_build()
    if build_dir is None:
        print("[make_release] ERROR: no build found. Run:")
        print("    cmake --build build --config Release")
        sys.exit(1)

    RELEASES_DIR.mkdir(parents=True, exist_ok=True)
    name = f"CRC-EIDOS-{version}-{platform}.zip"
    out = RELEASES_DIR / name

    files = collect_files(build_dir)
    if not files:
        print("[make_release] ERROR: nothing to package")
        sys.exit(1)

    total = 0
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for src, arc in files:
            zf.write(src, arc)
            total += src.stat().st_size

    print(f"[make_release] {out.name}")
    print(f"[make_release]   {len(files)} files, {total / 1048576:.1f} MB raw "
          f"-> {out.stat().st_size / 1048576:.1f} MB zipped")
    return out


def update_index(zip_path: Path, version: str, channel: str, platform: str) -> None:
    entries: list[dict] = []
    if INDEX_PATH.exists():
        try:
            data = json.loads(INDEX_PATH.read_text(encoding="utf-8"))
            entries = [e for e in data.get("releases", []) if e.get("file") != zip_path.name]
        except (json.JSONDecodeError, OSError):
            entries = []

    entries.append({
        "file": zip_path.name,
        "version": version,
        "channel": channel,
        "platform": platform,
        "exe": "EidosApp.exe",
        "size": zip_path.stat().st_size,
        "sha256": sha256_of(zip_path),
        "published": datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M"),
        "notes": read_notes(),
    })

    entries.sort(key=lambda e: e.get("published", ""), reverse=True)
    INDEX_PATH.write_text(
        json.dumps({"generated": datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC"),
                    "releases": entries}, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    print(f"[make_release] index: {INDEX_PATH}")
    print(f"[make_release] sha256: {entries[-1]['sha256'] if entries[-1]['file'] == zip_path.name else ''}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Package a CRC EIDOS build for the site")
    ap.add_argument("--version", default="", help="defaults to the ROADMAP version")
    ap.add_argument("--channel", default="beta", choices=["beta", "stable", "nightly"])
    ap.add_argument("--platform", default="win64")
    args = ap.parse_args()

    version = args.version or read_version()
    zip_path = build_zip(version, args.channel, args.platform)
    update_index(zip_path, version, args.channel, args.platform)
    print("[make_release] done - the launcher will pick it up from /api/releases")


if __name__ == "__main__":
    main()
