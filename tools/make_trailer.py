from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
FRAMES_DIR = PROJECT_ROOT / "trailer_frames"
OUT_DIR = PROJECT_ROOT / "public" / "site" / "static" / "video"

VIDEO_FILTER = (
    "scale=1920:1080:force_original_aspect_ratio=decrease,"
    "pad=1920:1080:(ow-iw)/2:(oh-ih)/2:color=black,"
    "setsar=1"
)

EXE_CANDIDATES = [
    PROJECT_ROOT / "build" / "bin" / "Release" / "EidosApp.exe",
    PROJECT_ROOT / "build" / "bin" / "Debug" / "EidosApp.exe",
]


def find_exe() -> Path | None:
    for c in EXE_CANDIDATES:
        if c.is_file():
            return c
    return None


def find_ffmpeg() -> str | None:
    direct = shutil.which("ffmpeg")
    if direct:
        return direct
    fallback = Path(os.environ.get("LOCALAPPDATA", "")) / "Microsoft" / "WinGet" / "Links" / "ffmpeg.exe"
    return str(fallback) if fallback.is_file() else None


def render(width: int, height: int, fps: int) -> int:
    exe = find_exe()
    if exe is None:
        print("[trailer] ERROR: no build. Run: cmake --build build --config Release")
        return 1

    if FRAMES_DIR.exists():
        shutil.rmtree(FRAMES_DIR, ignore_errors=True)
    FRAMES_DIR.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)
    env["EIDOS_TRAILER"] = "1"
    env["EIDOS_TRAILER_DIR"] = str(FRAMES_DIR)
    env["EIDOS_TRAILER_FPS"] = str(fps)
    env["EIDOS_TRAILER_W"] = str(width)
    env["EIDOS_TRAILER_H"] = str(height)

    print(f"[trailer] rendering {width}x{height} at {fps} fps")
    print("[trailer] the game window will take over - do not close it")

    started = time.time()
    proc = subprocess.run([str(exe)], cwd=str(PROJECT_ROOT), env=env)
    frames = sorted(FRAMES_DIR.glob("frame_*.png"))

    print(f"[trailer] {len(frames)} frames in {time.time() - started:.0f}s "
          f"(exit {proc.returncode})")
    return 0 if frames else 1


def encode(fps: int, name: str) -> int:
    ffmpeg = find_ffmpeg()
    if ffmpeg is None:
        print("[trailer] ERROR: ffmpeg not found")
        print("[trailer]   winget install Gyan.FFmpeg")
        return 1

    frames = sorted(FRAMES_DIR.glob("frame_*.png"))
    if not frames:
        print(f"[trailer] ERROR: no frames in {FRAMES_DIR}")
        return 1

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    mp4 = OUT_DIR / f"{name}.mp4"
    webm = OUT_DIR / f"{name}.webm"
    poster = OUT_DIR / f"{name}.jpg"

    common = [
        ffmpeg, "-y",
        "-framerate", str(fps),
        "-i", str(FRAMES_DIR / "frame_%05d.png"),
    ]

    print("[trailer] encoding mp4 (h264, YouTube-ready)")
    res = subprocess.run(common + [
        "-vf", VIDEO_FILTER,
        "-c:v", "libx264",
        "-preset", "slow",
        "-crf", "18",
        "-pix_fmt", "yuv420p",
        "-movflags", "+faststart",
        str(mp4),
    ], capture_output=True, text=True)
    if res.returncode != 0:
        print(res.stderr[-1500:])
        return res.returncode

    print("[trailer] encoding webm (for the site)")
    subprocess.run(common + [
        "-vf", VIDEO_FILTER,
        "-c:v", "libvpx-vp9",
        "-crf", "34",
        "-b:v", "0",
        "-row-mt", "1",
        str(webm),
    ], capture_output=True, text=True)

    mid = frames[len(frames) // 3]
    subprocess.run([ffmpeg, "-y", "-i", str(mid), "-q:v", "3", str(poster)],
                   capture_output=True, text=True)

    print(f"[trailer] {mp4}  ({mp4.stat().st_size / 1048576:.1f} MB)")
    if webm.exists():
        print(f"[trailer] {webm}  ({webm.stat().st_size / 1048576:.1f} MB)")
    if poster.exists():
        print(f"[trailer] {poster}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Render and encode the CRC EIDOS trailer")
    ap.add_argument("--width", type=int, default=1920)
    ap.add_argument("--height", type=int, default=1080)
    ap.add_argument("--fps", type=int, default=30)
    ap.add_argument("--name", default="crc-eidos-trailer")
    ap.add_argument("--skip-render", action="store_true",
                    help="reuse the frames already in trailer_frames/")
    ap.add_argument("--keep-frames", action="store_true")
    args = ap.parse_args()

    if not args.skip_render:
        if render(args.width, args.height, args.fps) != 0:
            return 1

    if encode(args.fps, args.name) != 0:
        return 1

    if not args.keep_frames:
        shutil.rmtree(FRAMES_DIR, ignore_errors=True)
        print("[trailer] frames cleaned up (--keep-frames to keep them)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
