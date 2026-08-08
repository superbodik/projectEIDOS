from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

LAUNCHER_DIR = Path(__file__).resolve().parent
DIST = LAUNCHER_DIR / "dist"
WORK = LAUNCHER_DIR / "build"
ICON = LAUNCHER_DIR / "icon.ico"
NAME = "CRC-EIDOS-Launcher"


def ensure_pyinstaller() -> bool:
    try:
        import PyInstaller  # noqa: F401
        return True
    except ImportError:
        pass

    print("[build] PyInstaller is missing, installing it...")
    res = subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"])
    if res.returncode != 0:
        print("[build] pip install pyinstaller failed")
        return False
    return True


def make_icon() -> Path | None:
    if ICON.exists():
        return ICON
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        print("[build] Pillow not installed - building without a custom icon")
        print("[build]   pip install pillow   to get the diamond icon")
        return None

    size = 256
    img = Image.new("RGBA", (size, size), (14, 12, 10, 255))
    draw = ImageDraw.Draw(img)
    c = size / 2
    r = size * 0.30
    draw.polygon([(c, c - r), (c + r, c), (c, c + r), (c - r, c)], fill=(217, 164, 65, 255))
    inner = r * 0.46
    draw.polygon([(c, c - inner), (c + inner, c), (c, c + inner), (c - inner, c)],
                 fill=(14, 12, 10, 255))
    img.save(ICON, sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print(f"[build] icon: {ICON}")
    return ICON


def build() -> int:
    if not ensure_pyinstaller():
        return 1

    ui_dir = LAUNCHER_DIR / "ui"
    if not ui_dir.is_dir():
        print(f"[build] ERROR: missing {ui_dir}")
        return 1

    for path in (DIST, WORK):
        shutil.rmtree(path, ignore_errors=True)

    icon = make_icon()

    cmd = [
        sys.executable, "-m", "PyInstaller",
        "--noconfirm",
        "--onefile",
        "--windowed",
        "--name", NAME,
        "--distpath", str(DIST),
        "--workpath", str(WORK),
        "--specpath", str(WORK),
        "--add-data", f"{ui_dir}{';' if sys.platform == 'win32' else ':'}ui",
        "--hidden-import", "webview",
        "--collect-all", "webview",
    ]
    if icon:
        cmd += ["--icon", str(icon)]
    cmd.append(str(LAUNCHER_DIR / "launcher.py"))

    print("[build] " + " ".join(cmd[:6]) + " ...")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("[build] PyInstaller failed")
        return res.returncode

    exe = DIST / f"{NAME}.exe"
    if not exe.exists():
        exe = DIST / NAME
    if not exe.exists():
        print("[build] ERROR: no executable produced")
        return 1

    print(f"[build] done: {exe}  ({exe.stat().st_size / 1048576:.1f} MB)")
    print("[build] the exe keeps its state next to itself in launcher_state.json")
    print("[build] point it at a live site with:  set EIDOS_SITE=https://your-domain")
    return 0


if __name__ == "__main__":
    raise SystemExit(build())
