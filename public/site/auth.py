from __future__ import annotations

import hashlib
import hmac
import os
import re
import secrets
import sqlite3
import threading
import time
from datetime import datetime, timedelta, timezone

PBKDF2_ITERATIONS = 210_000
SALT_BYTES = 16
TOKEN_BYTES = 32
SESSION_DAYS = 30

USERNAME_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{2,23}$")
EMAIL_RE = re.compile(r"^[^@\s]+@[^@\s]+\.[^@\s]{2,}$")

LOGIN_LIMIT = 8
LOGIN_WINDOW = 15 * 60

_login_lock = threading.Lock()
_login_hits: dict[str, list[float]] = {}


class AuthError(Exception):
    def __init__(self, code: str, message: str, status: int = 400):
        super().__init__(message)
        self.code = code
        self.message = message
        self.status = status


def login_rate_limited(ip: str) -> bool:
    now = time.time()
    with _login_lock:
        hits = [t for t in _login_hits.get(ip, []) if now - t < LOGIN_WINDOW]
        if len(hits) >= LOGIN_LIMIT:
            _login_hits[ip] = hits
            return True
        hits.append(now)
        _login_hits[ip] = hits
        return False


def clear_login_attempts(ip: str) -> None:
    with _login_lock:
        _login_hits.pop(ip, None)


def hash_password(password: str, salt: bytes | None = None,
                  iterations: int = PBKDF2_ITERATIONS) -> tuple[str, str, int]:
    if salt is None:
        salt = os.urandom(SALT_BYTES)
    derived = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return derived.hex(), salt.hex(), iterations


def verify_password(password: str, pw_hash: str, pw_salt: str, iterations: int) -> bool:
    try:
        salt = bytes.fromhex(pw_salt)
    except ValueError:
        return False
    derived = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return hmac.compare_digest(derived.hex(), pw_hash)


def init_schema(conn: sqlite3.Connection) -> None:
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE COLLATE NOCASE,
            email TEXT NOT NULL UNIQUE COLLATE NOCASE,
            pw_hash TEXT NOT NULL,
            pw_salt TEXT NOT NULL,
            pw_iters INTEGER NOT NULL,
            role TEXT NOT NULL DEFAULT 'player',
            lang TEXT NOT NULL DEFAULT 'en',
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            last_login TEXT
        )
        """
    )
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            source TEXT NOT NULL DEFAULT 'web',
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            expires_at TEXT NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        )
        """
    )
    conn.execute("CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id)")
    conn.commit()


def validate_registration(username: str, email: str, password: str) -> None:
    if not USERNAME_RE.match(username or ""):
        raise AuthError(
            "bad_username",
            "Username must be 3-24 characters: letters, digits, dot, dash or underscore.",
            422,
        )
    if not EMAIL_RE.match(email or ""):
        raise AuthError("bad_email", "Enter a valid email address.", 422)
    if len(password or "") < 8:
        raise AuthError("weak_password", "Password must be at least 8 characters.", 422)
    if len(password) > 200:
        raise AuthError("weak_password", "Password is too long.", 422)
    if password.lower() in {"password", "12345678", "qwertyui", "eidos123"}:
        raise AuthError("weak_password", "That password is too common.", 422)


def create_user(conn: sqlite3.Connection, username: str, email: str,
                password: str, lang: str = "en") -> dict:
    username = (username or "").strip()
    email = (email or "").strip().lower()
    validate_registration(username, email, password)

    pw_hash, pw_salt, iters = hash_password(password)
    try:
        cur = conn.execute(
            "INSERT INTO users (username, email, pw_hash, pw_salt, pw_iters, lang) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (username, email, pw_hash, pw_salt, iters, lang),
        )
        conn.commit()
    except sqlite3.IntegrityError as exc:
        detail = str(exc).lower()
        if "username" in detail:
            raise AuthError("username_taken", "That username is already taken.", 409) from exc
        raise AuthError("email_taken", "That email is already registered.", 409) from exc

    return {"id": cur.lastrowid, "username": username, "email": email, "role": "player"}


def find_user(conn: sqlite3.Connection, identifier: str) -> sqlite3.Row | None:
    return conn.execute(
        "SELECT * FROM users WHERE username = ? COLLATE NOCASE OR email = ? COLLATE NOCASE",
        (identifier, identifier.lower()),
    ).fetchone()


def login(conn: sqlite3.Connection, identifier: str, password: str,
          source: str = "web") -> tuple[str, dict]:
    identifier = (identifier or "").strip()
    row = find_user(conn, identifier)

    if row is None:
        hash_password(password or "x")
        raise AuthError("bad_credentials", "Wrong username or password.", 401)

    if not verify_password(password or "", row["pw_hash"], row["pw_salt"], row["pw_iters"]):
        raise AuthError("bad_credentials", "Wrong username or password.", 401)

    conn.execute("UPDATE users SET last_login = datetime('now') WHERE id = ?", (row["id"],))
    token = create_session(conn, row["id"], source)
    return token, user_public(row)


def create_session(conn: sqlite3.Connection, user_id: int, source: str) -> str:
    token = secrets.token_hex(TOKEN_BYTES)
    expires = datetime.now(tz=timezone.utc) + timedelta(days=SESSION_DAYS)
    conn.execute(
        "INSERT INTO sessions (token, user_id, source, expires_at) VALUES (?, ?, ?, ?)",
        (token, user_id, source, expires.strftime("%Y-%m-%d %H:%M:%S")),
    )
    conn.commit()
    return token


def session_user(conn: sqlite3.Connection, token: str) -> dict | None:
    if not token or len(token) != TOKEN_BYTES * 2:
        return None
    row = conn.execute(
        "SELECT u.*, s.expires_at, s.source FROM sessions s "
        "JOIN users u ON u.id = s.user_id WHERE s.token = ?",
        (token,),
    ).fetchone()
    if row is None:
        return None

    try:
        expires = datetime.strptime(row["expires_at"], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return None
    if expires < datetime.now(tz=timezone.utc).replace(tzinfo=None):
        conn.execute("DELETE FROM sessions WHERE token = ?", (token,))
        conn.commit()
        return None

    return user_public(row)


def logout(conn: sqlite3.Connection, token: str) -> None:
    if not token:
        return
    conn.execute("DELETE FROM sessions WHERE token = ?", (token,))
    conn.commit()


def purge_expired(conn: sqlite3.Connection) -> int:
    cur = conn.execute("DELETE FROM sessions WHERE expires_at < datetime('now')")
    conn.commit()
    return cur.rowcount or 0


def user_public(row) -> dict:
    return {
        "id": row["id"],
        "username": row["username"],
        "email": row["email"],
        "role": row["role"],
        "created_at": row["created_at"],
        "last_login": row["last_login"] if "last_login" in row.keys() else None,
    }


def count_users(conn: sqlite3.Connection) -> int:
    (count,) = conn.execute("SELECT COUNT(*) FROM users").fetchone()
    return int(count)
