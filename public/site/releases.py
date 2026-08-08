from __future__ import annotations

import hashlib
import json
import re
import threading
from datetime import datetime, timezone
from pathlib import Path

SITE_DIR = Path(__file__).resolve().parent
RELEASES_DIR = SITE_DIR / "releases"
INDEX_PATH = RELEASES_DIR / "index.json"

NAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._+-]{0,120}\.(zip|7z|tar\.gz|exe)$")
VERSION_RE = re.compile(r"(v\d+\.\d+(?:\.\d+)?)")

_lock = threading.Lock()
_hash_cache: dict[str, tuple[float, str]] = {}


def sha256_of(path: Path) -> str:
    key = str(path)
    stamp = path.stat().st_mtime
    with _lock:
        cached = _hash_cache.get(key)
        if cached and cached[0] == stamp:
            return cached[1]

    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    value = digest.hexdigest()

    with _lock:
        _hash_cache[key] = (stamp, value)
    return value


def _load_index() -> dict:
    if not INDEX_PATH.exists():
        return {}
    try:
        data = json.loads(INDEX_PATH.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}
    if not isinstance(data, dict):
        return {}
    return {entry.get("file"): entry for entry in data.get("releases", []) if entry.get("file")}


def safe_file(name: str) -> Path | None:
    if not NAME_RE.match(name or ""):
        return None
    target = (RELEASES_DIR / name).resolve()
    try:
        target.relative_to(RELEASES_DIR.resolve())
    except ValueError:
        return None
    if not target.is_file():
        return None
    return target


def list_releases(with_hash: bool = True) -> list[dict]:
    if not RELEASES_DIR.is_dir():
        return []

    meta = _load_index()
    out = []

    for path in RELEASES_DIR.iterdir():
        if not path.is_file() or not NAME_RE.match(path.name):
            continue

        info = dict(meta.get(path.name, {}))
        stat = path.stat()
        version = info.get("version")
        if not version:
            m = VERSION_RE.search(path.name)
            version = m.group(1) if m else "unknown"

        entry = {
            "file": path.name,
            "version": version,
            "channel": info.get("channel", "beta"),
            "platform": info.get("platform", "windows" if "win" in path.name.lower() else "any"),
            "size": stat.st_size,
            "size_mb": round(stat.st_size / (1024 * 1024), 1),
            "published": info.get(
                "published",
                datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).strftime("%Y-%m-%d %H:%M"),
            ),
            "notes": info.get("notes", ""),
            "exe": info.get("exe", "EidosApp.exe"),
            "url": f"/download/{path.name}",
        }
        if with_hash:
            entry["sha256"] = info.get("sha256") or sha256_of(path)
        out.append(entry)

    def sort_key(e: dict):
        parts = re.findall(r"\d+", e["version"])
        nums = tuple(int(p) for p in parts) + (0, 0, 0)
        return (nums[:3], e["published"])

    out.sort(key=sort_key, reverse=True)
    return out


def latest(platform: str = "") -> dict | None:
    items = list_releases()
    if platform:
        wanted = [r for r in items if r["platform"] == platform]
        if wanted:
            return wanted[0]
    return items[0] if items else None


def write_index(entries: list[dict]) -> None:
    RELEASES_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "generated": datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC"),
        "releases": entries,
    }
    INDEX_PATH.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
