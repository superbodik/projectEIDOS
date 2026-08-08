from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
import webbrowser
import zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

if getattr(sys, "frozen", False):
    LAUNCHER_DIR = Path(sys.executable).resolve().parent
    BUNDLE_DIR = Path(getattr(sys, "_MEIPASS", LAUNCHER_DIR))
else:
    LAUNCHER_DIR = Path(__file__).resolve().parent
    BUNDLE_DIR = LAUNCHER_DIR

PROJECT_ROOT = LAUNCHER_DIR.parent.parent
UI_DIR = BUNDLE_DIR / "ui"
STATE_PATH = LAUNCHER_DIR / "launcher_state.json"
GAMES_DIR = LAUNCHER_DIR / "games"

PORT = int(os.environ.get("LAUNCHER_PORT", "8777"))
SITE_URL = os.environ.get("EIDOS_SITE", "http://127.0.0.1:8001").rstrip("/")
DISCORD_URL = "https://discord.gg/d7qz94Y9Bk"
TELEGRAM_URL = "https://t.me/CRCEIODOS"

DEV_EXE_CANDIDATES = [
    PROJECT_ROOT / "build" / "bin" / "Release" / "EidosApp.exe",
    PROJECT_ROOT / "build" / "bin" / "Debug" / "EidosApp.exe",
]

_state_lock = threading.Lock()
_running: subprocess.Popen | None = None
_run_started: float = 0.0

_job_lock = threading.Lock()
_job = {"active": False, "phase": "idle", "percent": 0.0,
        "message": "", "error": "", "version": ""}


def load_state() -> dict:
    if STATE_PATH.exists():
        try:
            return json.loads(STATE_PATH.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            pass
    return {"playtime_seconds": 0, "launches": 0, "last_played": None,
            "token": "", "username": "", "installed": {}}


def save_state(state: dict) -> None:
    try:
        STATE_PATH.write_text(json.dumps(state, indent=2), encoding="utf-8")
    except OSError as exc:
        print(f"[launcher] could not save state: {exc}")


def set_job(**kw) -> None:
    with _job_lock:
        _job.update(kw)


def get_job() -> dict:
    with _job_lock:
        return dict(_job)


def api(path: str, payload: dict | None = None, token: str = "",
        timeout: float = 15.0) -> tuple[int, dict]:
    url = f"{SITE_URL}{path}"
    data = json.dumps(payload).encode("utf-8") if payload is not None else None
    req = urllib.request.Request(url, data=data, method="POST" if data else "GET")
    req.add_header("Accept", "application/json")
    req.add_header("User-Agent", "eidos-launcher/2.0")
    if data:
        req.add_header("Content-Type", "application/json")
    if token:
        req.add_header("Authorization", f"Bearer {token}")

    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read().decode("utf-8", errors="replace")
            return resp.status, json.loads(body) if body else {}
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        try:
            return exc.code, json.loads(body) if body else {}
        except json.JSONDecodeError:
            return exc.code, {"message": body[:200] or exc.reason}
    except urllib.error.URLError as exc:
        return 0, {"message": f"Cannot reach {SITE_URL} - is the site running?"}
    except (TimeoutError, json.JSONDecodeError) as exc:
        return 0, {"message": str(exc)}


def installed_entry(state: dict) -> dict | None:
    inst = state.get("installed") or {}
    exe = inst.get("exe")
    if exe and Path(exe).is_file():
        return inst
    return None


def dev_exe() -> Path | None:
    for c in DEV_EXE_CANDIDATES:
        if c.is_file():
            return c
    return None


def active_exe(state: dict) -> tuple[Path | None, str, bool]:
    inst = installed_entry(state)
    if inst:
        return Path(inst["exe"]), inst.get("version", "unknown"), False
    dev = dev_exe()
    if dev:
        return dev, "dev build", True
    return None, "", False


def find_exe_in(root: Path) -> Path | None:
    direct = root / "EidosApp.exe"
    if direct.is_file():
        return direct
    for path in root.rglob("EidosApp.exe"):
        return path
    return None


def download_and_install(version: str, url: str, sha256: str, token: str) -> None:
    set_job(active=True, phase="download", percent=0.0, error="",
            message="Connecting...", version=version)

    target_dir = GAMES_DIR / version
    tmp_zip = GAMES_DIR / f".{version}.part"
    GAMES_DIR.mkdir(parents=True, exist_ok=True)

    try:
        req = urllib.request.Request(f"{SITE_URL}{url}" if url.startswith("/") else url)
        req.add_header("User-Agent", "eidos-launcher/2.0")
        if token:
            req.add_header("Authorization", f"Bearer {token}")

        digest = hashlib.sha256()
        with urllib.request.urlopen(req, timeout=60) as resp:
            total = int(resp.headers.get("Content-Length") or 0)
            done = 0
            with tmp_zip.open("wb") as fh:
                while True:
                    chunk = resp.read(256 * 1024)
                    if not chunk:
                        break
                    fh.write(chunk)
                    digest.update(chunk)
                    done += len(chunk)
                    pct = (done / total * 100.0) if total else 0.0
                    set_job(percent=round(pct, 1),
                            message=f"{done / 1048576:.1f} / {total / 1048576:.1f} MB"
                            if total else f"{done / 1048576:.1f} MB")

        if sha256 and digest.hexdigest() != sha256:
            tmp_zip.unlink(missing_ok=True)
            set_job(active=False, phase="error", error="Checksum mismatch - download corrupted")
            return

        set_job(phase="extract", percent=100.0, message="Extracting...")
        if target_dir.exists():
            shutil.rmtree(target_dir, ignore_errors=True)
        target_dir.mkdir(parents=True, exist_ok=True)

        with zipfile.ZipFile(tmp_zip) as zf:
            for member in zf.namelist():
                dest = (target_dir / member).resolve()
                if not str(dest).startswith(str(target_dir.resolve())):
                    raise ValueError(f"unsafe path in archive: {member}")
            zf.extractall(target_dir)

        tmp_zip.unlink(missing_ok=True)

        exe = find_exe_in(target_dir)
        if exe is None:
            set_job(active=False, phase="error", error="No EidosApp.exe inside the archive")
            return

        with _state_lock:
            state = load_state()
            state["installed"] = {
                "version": version,
                "dir": str(target_dir),
                "exe": str(exe),
                "sha256": sha256,
                "installed_at": time.strftime("%Y-%m-%d %H:%M"),
            }
            save_state(state)

        set_job(active=False, phase="done", percent=100.0,
                message=f"{version} installed", error="")

    except (urllib.error.URLError, OSError, ValueError, zipfile.BadZipFile) as exc:
        tmp_zip.unlink(missing_ok=True)
        set_job(active=False, phase="error", error=str(exc)[:200])


def snapshot() -> dict:
    global _running

    state = load_state()
    token = state.get("token", "")

    with _state_lock:
        running = _running is not None and _running.poll() is None
        live_seconds = (time.time() - _run_started) if running else 0.0

    total = state.get("playtime_seconds", 0) + live_seconds
    exe, version, is_dev = active_exe(state)

    user = None
    if token:
        status, body = api("/api/auth/me", token=token)
        if status == 200:
            user = body.get("user")
        elif status == 401:
            state["token"] = ""
            state["username"] = ""
            save_state(state)
            token = ""

    remote = None
    downloads_open = True
    gate_message = ""
    status, body = api("/api/releases/latest")
    if status == 200:
        remote = body
    elif status == 403:
        downloads_open = False
        gate_message = body.get("message", "Downloads are closed for now.")
    elif status == 404:
        gate_message = body.get("message", "No build has been published yet.")

    saves_dir = PROJECT_ROOT / "saves"
    if exe and not is_dev:
        saves_dir = Path(exe).parent / "saves"
    worlds = []
    if saves_dir.is_dir():
        for d in sorted(saves_dir.iterdir()):
            if d.is_dir() and (d / "level.dat").exists() and d.name != "MenuPanorama":
                worlds.append(d.name)

    update_available = bool(
        remote and exe and not is_dev and remote.get("version") != version
    )

    return {
        "site": SITE_URL,
        "signed_in": user is not None,
        "user": user,
        "installed": exe is not None,
        "is_dev_build": is_dev,
        "exe": str(exe) if exe else "",
        "version": version,
        "size_mb": round(exe.stat().st_size / 1048576, 1) if exe else 0.0,
        "remote": remote,
        "downloads_open": downloads_open,
        "gate_message": gate_message,
        "update_available": update_available,
        "running": running,
        "job": get_job(),
        "playtime_hours": round(total / 3600.0, 1),
        "launches": state.get("launches", 0),
        "last_played": state.get("last_played"),
        "worlds": worlds,
        "discord": DISCORD_URL,
        "telegram": TELEGRAM_URL,
    }


def watch_process(proc: subprocess.Popen, started: float) -> None:
    global _running
    proc.wait()
    elapsed = time.time() - started
    with _state_lock:
        state = load_state()
        state["playtime_seconds"] = state.get("playtime_seconds", 0) + elapsed
        state["launches"] = state.get("launches", 0) + 1
        state["last_played"] = time.strftime("%Y-%m-%d %H:%M")
        save_state(state)
        _running = None
    print(f"[launcher] game exited after {elapsed:.0f}s")


def launch_game() -> tuple[bool, str]:
    global _running, _run_started

    with _state_lock:
        if _running is not None and _running.poll() is None:
            return False, "already running"

    state = load_state()
    exe, _, _ = active_exe(state)
    if exe is None:
        return False, "Game is not installed yet"

    env = dict(os.environ)
    if state.get("username"):
        env["EIDOS_PLAYER"] = state["username"]
    if state.get("token"):
        env["EIDOS_TOKEN"] = state["token"]

    try:
        proc = subprocess.Popen([str(exe)], cwd=str(exe.parent), env=env)
    except OSError as exc:
        return False, f"failed to start: {exc}"

    with _state_lock:
        _running = proc
        _run_started = time.time()

    threading.Thread(target=watch_process, args=(proc, _run_started), daemon=True).start()
    return True, "launched"


MIME = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".svg": "image/svg+xml",
    ".png": "image/png",
    ".json": "application/json; charset=utf-8",
}


class Handler(BaseHTTPRequestHandler):
    server_version = "eidos-launcher/2.0"

    def log_message(self, fmt, *args):
        pass

    def _send(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _json(self, status: int, payload: dict) -> None:
        self._send(status, json.dumps(payload).encode("utf-8"), "application/json")

    def _body(self) -> dict:
        length = int(self.headers.get("Content-Length") or 0)
        if not length:
            return {}
        try:
            return json.loads(self.rfile.read(length).decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            return {}

    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/api/state":
            self._json(200, snapshot())
            return

        rel = "index.html" if path == "/" else path.lstrip("/")
        target = (UI_DIR / rel).resolve()
        if not str(target).startswith(str(UI_DIR.resolve())) or not target.is_file():
            self._json(404, {"error": "not found"})
            return

        self._send(200, target.read_bytes(), MIME.get(target.suffix, "application/octet-stream"))

    def do_POST(self):
        path = urlparse(self.path).path
        body = self._body()

        if path == "/api/login":
            status, data = api("/api/auth/login", {
                "identifier": body.get("identifier", ""),
                "password": body.get("password", ""),
                "source": "launcher",
            })
            if status == 200 and data.get("token"):
                with _state_lock:
                    state = load_state()
                    state["token"] = data["token"]
                    state["username"] = data.get("user", {}).get("username", "")
                    save_state(state)
                self._json(200, {"ok": True, "user": data.get("user")})
            else:
                self._json(status or 502,
                           {"ok": False, "message": data.get("message", "Sign-in failed")})
            return

        if path == "/api/register":
            status, data = api("/api/auth/register", {
                "username": body.get("username", ""),
                "email": body.get("email", ""),
                "password": body.get("password", ""),
                "lang": "en",
            })
            if status == 201 and data.get("token"):
                with _state_lock:
                    state = load_state()
                    state["token"] = data["token"]
                    state["username"] = data.get("user", {}).get("username", "")
                    save_state(state)
                self._json(201, {"ok": True, "user": data.get("user")})
            else:
                self._json(status or 502,
                           {"ok": False, "message": data.get("message", "Registration failed")})
            return

        if path == "/api/logout":
            state = load_state()
            api("/api/auth/logout", {}, token=state.get("token", ""))
            with _state_lock:
                state = load_state()
                state["token"] = ""
                state["username"] = ""
                save_state(state)
            self._json(200, {"ok": True})
            return

        if path == "/api/install":
            job = get_job()
            if job["active"]:
                self._json(409, {"ok": False, "message": "A download is already running"})
                return

            state = load_state()
            token = state.get("token", "")
            if not token:
                self._json(401, {"ok": False, "message": "Sign in first"})
                return

            status, rel = api("/api/releases/latest")
            if status != 200:
                self._json(status or 502,
                           {"ok": False, "message": rel.get("message", "No release available")})
                return

            threading.Thread(
                target=download_and_install,
                args=(rel.get("version", "unknown"), rel.get("url", ""),
                      rel.get("sha256", ""), token),
                daemon=True,
            ).start()
            self._json(200, {"ok": True, "version": rel.get("version")})
            return

        if path == "/api/launch":
            ok, msg = launch_game()
            self._json(200 if ok else 409, {"ok": ok, "message": msg})
            return

        if path == "/api/quit":
            self._json(200, {"ok": True})
            threading.Thread(target=lambda: (time.sleep(0.3), os._exit(0)), daemon=True).start()
            return

        self._json(404, {"error": "not found"})


def main() -> None:
    if not UI_DIR.is_dir():
        print(f"[launcher] missing UI directory: {UI_DIR}")
        sys.exit(1)

    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    url = f"http://127.0.0.1:{PORT}/"
    print(f"[launcher] CRC EIDOS launcher on {url}")
    print(f"[launcher] site: {SITE_URL}")
    print(f"[launcher] games: {GAMES_DIR}")

    try:
        import webview

        threading.Thread(target=server.serve_forever, daemon=True).start()
        webview.create_window("CRC EIDOS Launcher", url, width=1280, height=820,
                              background_color="#0e0c0a")
        webview.start()
        os._exit(0)
    except ImportError:
        print("[launcher] pywebview not installed - opening in browser instead")
        threading.Timer(0.6, lambda: webbrowser.open(url)).start()
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            server.shutdown()


if __name__ == "__main__":
    main()
