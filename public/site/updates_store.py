from __future__ import annotations

import json
import sqlite3
from datetime import datetime, timezone

SECTIONS = ("added", "fixed", "changed", "known")
CHANNELS = ("beta", "stable", "nightly")


class UpdateError(Exception):
    def __init__(self, message: str, status: int = 400):
        super().__init__(message)
        self.message = message
        self.status = status


def init_schema(conn: sqlite3.Connection) -> None:
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS updates (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            version      TEXT NOT NULL,
            title        TEXT NOT NULL,
            channel      TEXT NOT NULL DEFAULT 'beta',
            summary      TEXT NOT NULL DEFAULT '',
            body_json    TEXT NOT NULL DEFAULT '{}',
            published_at TEXT NOT NULL,
            created_at   TEXT NOT NULL DEFAULT (datetime('now'))
        )
        """
    )
    conn.execute(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_updates_version "
        "ON updates(version, channel)"
    )
    conn.commit()


def _clean_list(value) -> list[str]:
    if value is None:
        return []
    if isinstance(value, str):
        value = [value]
    if not isinstance(value, list):
        raise UpdateError("added/fixed/changed/known must be lists of strings", 422)
    out = []
    for item in value:
        text = str(item).strip()
        if text:
            out.append(text[:400])
    return out[:60]


def publish(conn: sqlite3.Connection, payload: dict) -> dict:
    version = str(payload.get("version", "")).strip()
    title = str(payload.get("title", "")).strip()
    if not version:
        raise UpdateError("version is required", 422)
    if not title:
        raise UpdateError("title is required", 422)

    channel = str(payload.get("channel", "beta")).strip().lower()
    if channel not in CHANNELS:
        raise UpdateError(f"channel must be one of {', '.join(CHANNELS)}", 422)

    body = {s: _clean_list(payload.get(s)) for s in SECTIONS}
    if not any(body.values()):
        raise UpdateError("an update needs at least one added/fixed/changed/known entry", 422)

    published_at = str(payload.get("published_at", "")).strip()
    if not published_at:
        published_at = datetime.now(tz=timezone.utc).strftime("%Y-%m-%d %H:%M")

    summary = str(payload.get("summary", "")).strip()[:600]

    conn.execute(
        """
        INSERT INTO updates (version, title, channel, summary, body_json, published_at)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(version, channel) DO UPDATE SET
            title = excluded.title,
            summary = excluded.summary,
            body_json = excluded.body_json,
            published_at = excluded.published_at
        """,
        (version, title, channel, summary,
         json.dumps(body, ensure_ascii=False), published_at),
    )
    conn.commit()

    return get_one(conn, version, channel) or {}


def _row_to_dict(row: sqlite3.Row) -> dict:
    try:
        body = json.loads(row["body_json"])
    except (json.JSONDecodeError, TypeError):
        body = {}
    entry = {
        "version": row["version"],
        "title": row["title"],
        "channel": row["channel"],
        "summary": row["summary"],
        "published_at": row["published_at"],
    }
    for s in SECTIONS:
        entry[s] = body.get(s, [])
    entry["total"] = sum(len(entry[s]) for s in SECTIONS)
    return entry


def listing(conn: sqlite3.Connection, limit: int = 20, channel: str = "") -> list[dict]:
    limit = max(1, min(int(limit), 100))
    if channel:
        rows = conn.execute(
            "SELECT * FROM updates WHERE channel = ? "
            "ORDER BY published_at DESC, id DESC LIMIT ?",
            (channel, limit),
        ).fetchall()
    else:
        rows = conn.execute(
            "SELECT * FROM updates ORDER BY published_at DESC, id DESC LIMIT ?",
            (limit,),
        ).fetchall()
    return [_row_to_dict(r) for r in rows]


def get_one(conn: sqlite3.Connection, version: str, channel: str = "") -> dict | None:
    if channel:
        row = conn.execute(
            "SELECT * FROM updates WHERE version = ? AND channel = ?",
            (version, channel),
        ).fetchone()
    else:
        row = conn.execute(
            "SELECT * FROM updates WHERE version = ? ORDER BY id DESC LIMIT 1",
            (version,),
        ).fetchone()
    return _row_to_dict(row) if row else None


def latest(conn: sqlite3.Connection, channel: str = "") -> dict | None:
    items = listing(conn, 1, channel)
    return items[0] if items else None


def delete(conn: sqlite3.Connection, version: str, channel: str) -> bool:
    cur = conn.execute("DELETE FROM updates WHERE version = ? AND channel = ?",
                       (version, channel))
    conn.commit()
    return (cur.rowcount or 0) > 0
