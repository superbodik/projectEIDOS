"""Shared DB access and security helpers for app.py (site) and api.py (API).

Both processes open the same SQLite file. WAL mode lets one process write
while the other reads without the "database is locked" errors plain
rollback-journal mode gives under two-process access.
"""
from __future__ import annotations

import hmac
import os
import sqlite3
import threading
import time
from pathlib import Path
from typing import Optional

from fastapi import Header, HTTPException

BASE_DIR = Path(__file__).resolve().parent


def load_dotenv() -> None:
    env_path = BASE_DIR / ".env"
    if not env_path.exists():
        return
    for line in env_path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, value = line.partition("=")
        key, value = key.strip(), value.strip()
        if key and key not in os.environ:
            os.environ[key] = value


load_dotenv()


def _resolve_db_path(raw: str) -> str:
    # DB_PATH in .env is written as a bare "beta_signups.db" - a relative
    # path resolves against the process's CWD, not this file's folder. If
    # a host process manager launches uvicorn from a different working
    # directory, this used to silently point at a different (often
    # freshly-created, empty) database with no error - "I updated
    # everything and nothing changed" is exactly what that looks like.
    p = Path(raw)
    return str(p) if p.is_absolute() else str(BASE_DIR / p)


DB_PATH = _resolve_db_path(os.environ.get("DB_PATH", "beta_signups.db"))
ADMIN_TOKEN = os.environ.get("ADMIN_TOKEN", "")
TRUST_PROXY = os.environ.get("TRUST_PROXY", "0") == "1"

_db_lock = threading.Lock()


def log(tag: str, msg: str) -> None:
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{ts}] [{tag}] {msg}", flush=True)


def startup_report(tag: str) -> None:
    log(tag, f"process pid={os.getpid()} cwd={os.getcwd()}")
    log(tag, f"DB_PATH resolved to: {DB_PATH}")
    exists = Path(DB_PATH).exists()
    log(tag, f"DB file exists: {exists}"
        + ("" if exists else " <-- fresh/empty DB, is this really the file you meant?"))
    if exists:
        try:
            with db() as conn:
                tables = ["signups", "updates", "users", "sessions"]
                counts = []
                for tname in tables:
                    try:
                        (n,) = conn.execute(f"SELECT COUNT(*) FROM {tname}").fetchone()
                        counts.append(f"{tname}={n}")
                    except sqlite3.OperationalError:
                        counts.append(f"{tname}=<no table yet>")
                log(tag, "row counts: " + ", ".join(counts))
        except Exception as exc:  # startup diagnostics must never crash the app
            log(tag, f"could not read row counts: {exc}")
    log(tag, f"ADMIN_TOKEN set: {bool(ADMIN_TOKEN)}")


def db() -> sqlite3.Connection:
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA busy_timeout=5000")
    return conn


def init_schema(conn: sqlite3.Connection) -> None:
    """Idempotent - safe to call from both processes at startup regardless
    of which one happens to start first."""
    import auth
    import updates_store

    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS signups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            lang TEXT NOT NULL DEFAULT 'en',
            platform TEXT,
            message TEXT,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
        """
    )
    conn.execute("CREATE INDEX IF NOT EXISTS idx_signups_created ON signups(created_at)")
    conn.commit()
    auth.init_schema(conn)
    updates_store.init_schema(conn)


def latest_release_stamp() -> tuple[str, str]:
    """Version/date shown in page headers - from the published-updates
    table, not ROADMAP.md (the deployed site container has no reason to
    ship the engine's source tree, so parsing it there returns nothing).
    Shared by app.py and api.py so both report the same version."""
    import updates_store

    with _db_lock, db() as conn:
        row = updates_store.latest(conn, channel="beta")
    if not row:
        return "—", "—"
    return row["version"], row["published_at"]


def client_ip(request) -> str:
    # Only trust X-Forwarded-For when we know a reverse proxy sits in
    # front of us - otherwise a client can spoof the header and dodge
    # rate limiting entirely.
    if TRUST_PROXY:
        fwd = request.headers.get("x-forwarded-for", "")
        if fwd:
            return fwd.split(",")[0].strip()
    return request.client.host if request.client else "unknown"


def require_admin(authorization: Optional[str] = Header(default=None)) -> None:
    if not ADMIN_TOKEN:
        raise HTTPException(status_code=404, detail="not found")
    expected = f"Bearer {ADMIN_TOKEN}"
    given = authorization or ""
    # Constant-time compare - a naive `!=` leaks how many leading bytes
    # matched through response timing, letting an attacker recover the
    # token byte by byte.
    if not hmac.compare_digest(given, expected):
        raise HTTPException(status_code=401, detail="unauthorized")


class RateLimiter:
    """Per-IP sliding-window limiter, one instance per endpoint class."""

    def __init__(self, limit: int, window_seconds: float):
        self.limit = limit
        self.window = window_seconds
        self._lock = threading.Lock()
        self._hits: dict[str, list[float]] = {}

    def hit(self, key: str) -> bool:
        """Record a hit; returns True if the caller is over the limit."""
        now = time.time()
        with self._lock:
            hits = [t for t in self._hits.get(key, []) if now - t < self.window]
            if len(hits) >= self.limit:
                self._hits[key] = hits
                return True
            hits.append(now)
            self._hits[key] = hits
            return False
