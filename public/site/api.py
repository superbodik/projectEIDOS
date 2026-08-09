"""CRC EIDOS service API - updates feed, wiki data, releases, admin.

Runs as its own process on API_PORT (default 2976), separate from the
public website (app.py, port 8001 by default). Split out so the JSON API
can sit on its own subdomain (api.eidos.pp.ua) with its own rate limits
and CORS policy, without the marketing pages sharing its blast radius or
vice versa.

Auth (/api/auth/*, /api/signup) stays on the site: those are same-origin
cookie-session endpoints for the site's own login form. Everything here
is either public read-only, or admin-token protected - neither depends
on browser cookies, so it is free to live cross-origin.
"""
from __future__ import annotations

import json
import os
import time
from contextlib import asynccontextmanager

from fastapi import Depends, FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from starlette.middleware.base import BaseHTTPMiddleware

import apicommon
import releases as release_store
import updates_store
import wiki_data

API_PORT = int(os.environ.get("API_PORT", "2976"))
DOWNLOADS_ENABLED = os.environ.get("DOWNLOADS_ENABLED", "0") == "1"

# Comma-separated list of origins allowed to call this API from a browser.
# Empty means "no browser origin is allowed" - server-to-server callers
# (the launcher, publish_update.py, curl) are unaffected by CORS either way.
_origins_env = os.environ.get("API_CORS_ORIGINS", "").strip()
CORS_ORIGINS = [o.strip() for o in _origins_env.split(",") if o.strip()]

# The wiki snapshot alone is ~130KB at 151 blocks and only grows as more
# content is added, so the cap has to sit well above any single JSON
# payload this API legitimately accepts while still refusing anything
# resembling a file upload (which has its own endpoint with its own rules).
MAX_BODY_BYTES = 4 * 1024 * 1024

_read_limit = apicommon.RateLimiter(limit=120, window_seconds=60)
_write_limit = apicommon.RateLimiter(limit=20, window_seconds=60)


class SecurityMiddleware(BaseHTTPMiddleware):
    """Body-size cap, per-IP rate limiting, and standard response headers.

    One middleware instead of four so every route gets all of it by
    construction - a route added later can't accidentally skip the checks
    by forgetting a decorator.
    """

    async def dispatch(self, request: Request, call_next):
        started = time.time()
        ip = apicommon.client_ip(request)

        content_length = request.headers.get("content-length")
        if content_length and int(content_length) > MAX_BODY_BYTES:
            apicommon.log("eidos-api", f"413 {request.method} {request.url.path} ip={ip} "
                          f"(body too large: {content_length} bytes)")
            return JSONResponse({"error": "payload_too_large"}, status_code=413)

        limiter = _write_limit if request.method in ("POST", "PUT", "DELETE", "PATCH") else _read_limit
        if limiter.hit(ip):
            apicommon.log("eidos-api", f"429 {request.method} {request.url.path} ip={ip}")
            return JSONResponse({"error": "rate_limited",
                                 "message": "Too many requests."}, status_code=429)

        response = await call_next(request)
        ms = (time.time() - started) * 1000
        apicommon.log("eidos-api",
                      f"{response.status_code} {request.method} {request.url.path} "
                      f"ip={ip} {ms:.0f}ms")

        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["X-Frame-Options"] = "DENY"
        response.headers["Referrer-Policy"] = "no-referrer"
        response.headers["Cache-Control"] = response.headers.get(
            "Cache-Control", "no-store")
        return response


@asynccontextmanager
async def lifespan(app: FastAPI):
    apicommon.startup_report("eidos-api")
    with apicommon._db_lock, apicommon.db() as conn:
        apicommon.init_schema(conn)
    apicommon.log("eidos-api", f"listening on 0.0.0.0:{API_PORT}")
    if not apicommon.ADMIN_TOKEN:
        apicommon.log("eidos-api", "WARNING: ADMIN_TOKEN unset - admin routes are disabled (404)")
    if not CORS_ORIGINS:
        apicommon.log("eidos-api", "no API_CORS_ORIGINS set - browser pages cannot call this "
                       "API (server-to-server callers are unaffected)")
    else:
        apicommon.log("eidos-api", f"CORS allowed origins: {CORS_ORIGINS}")
    yield


app = FastAPI(title="CRC EIDOS API", lifespan=lifespan, docs_url=None, redoc_url=None)
app.add_middleware(SecurityMiddleware)
if CORS_ORIGINS:
    app.add_middleware(
        CORSMiddleware,
        allow_origins=CORS_ORIGINS,
        allow_methods=["GET", "POST", "DELETE"],
        allow_headers=["Authorization", "Content-Type"],
        allow_credentials=False,
    )


@app.get("/health")
async def health():
    return {"status": "ok", "service": "api"}


@app.get("/api/updates")
async def api_updates(limit: int = 20, channel: str = ""):
    with apicommon._db_lock, apicommon.db() as conn:
        items = updates_store.listing(conn, limit, channel)
    return {"count": len(items), "updates": items}


@app.get("/api/updates/latest")
async def api_updates_latest(channel: str = ""):
    with apicommon._db_lock, apicommon.db() as conn:
        item = updates_store.latest(conn, channel)
    if item is None:
        return JSONResponse({"error": "no_updates",
                             "message": "Nothing published yet."}, status_code=404)
    return item


@app.get("/api/updates/{version}")
async def api_update_one(version: str, channel: str = ""):
    with apicommon._db_lock, apicommon.db() as conn:
        item = updates_store.get_one(conn, version, channel)
    if item is None:
        return JSONResponse({"error": "not_found",
                             "message": f"No changelog for {version}."}, status_code=404)
    return item


@app.post("/api/admin/updates", dependencies=[Depends(apicommon.require_admin)])
async def api_publish_update(payload: dict):
    try:
        with apicommon._db_lock, apicommon.db() as conn:
            entry = updates_store.publish(conn, payload)
    except updates_store.UpdateError as exc:
        return JSONResponse({"error": "invalid", "message": exc.message},
                            status_code=exc.status)
    return JSONResponse({"ok": True, "update": entry}, status_code=201)


@app.delete("/api/admin/updates/{version}", dependencies=[Depends(apicommon.require_admin)])
async def api_delete_update(version: str, channel: str = "beta"):
    with apicommon._db_lock, apicommon.db() as conn:
        removed = updates_store.delete(conn, version, channel)
    if not removed:
        return JSONResponse({"error": "not_found"}, status_code=404)
    return {"ok": True}


@app.get("/api/wiki")
async def api_wiki():
    data = wiki_data.get()
    version, updated = apicommon.latest_release_stamp()
    return {
        "version": version,
        "updated": updated,
        "foods": data["foods"],
        "quests": data["quests"],
        "suites": data["suites"],
        "ores": data["ores"],
        "biomes": data["biomes"],
        "blocks": data["blocks"],
    }


@app.get("/api/wiki/blocks")
async def api_wiki_blocks():
    data = wiki_data.get()
    return {"count": len(data["block_details"]), "blocks": data["block_details"]}


@app.get("/api/wiki/blocks/{block_id}")
async def api_wiki_block_one(block_id: int):
    data = wiki_data.get()
    for b in data["block_details"]:
        if b["id"] == block_id:
            return b
    return JSONResponse({"error": "not_found",
                         "message": f"No block with id {block_id}."}, status_code=404)


@app.get("/api/wiki/biomes")
async def api_wiki_biomes():
    data = wiki_data.get()
    details = data["biome_details"]
    return {
        "count": len(data["biomes"]),
        "biomes": [{"name": name, **details.get(name, {})} for name in data["biomes"]],
    }


@app.get("/api/wiki/biomes/{name}")
async def api_wiki_biome_one(name: str):
    data = wiki_data.get()
    details = data["biome_details"]
    if name not in details and name not in data["biomes"]:
        return JSONResponse({"error": "not_found",
                             "message": f"No biome named {name}."}, status_code=404)
    return {"name": name, **details.get(name, {})}


WIKI_REQUIRED_KEYS = {
    "blocks", "block_details", "block_groups", "foods", "quests",
    "eras", "suites", "ores", "biomes", "biome_details", "controls", "commands",
}


@app.post("/api/admin/wiki", dependencies=[Depends(apicommon.require_admin)])
async def api_publish_wiki(payload: dict):
    # Sanity-check the shape before overwriting the live snapshot - a
    # malformed push would otherwise take the whole /wiki page down until
    # someone notices and re-pushes a good one.
    missing = WIKI_REQUIRED_KEYS - payload.keys()
    if missing:
        return JSONResponse({"error": "invalid",
                             "message": f"missing keys: {sorted(missing)}"}, status_code=422)
    if not isinstance(payload.get("block_details"), list) or not payload["block_details"]:
        return JSONResponse({"error": "invalid",
                             "message": "block_details must be a non-empty list"}, status_code=422)

    wiki_data.WIKI_SNAPSHOT_PATH.write_text(
        json.dumps(payload, ensure_ascii=False, indent=1), encoding="utf-8")
    apicommon.log("eidos-api", f"wiki snapshot replaced: "
                  f"{len(payload['block_details'])} blocks, {len(payload.get('biomes', []))} biomes")
    return {"ok": True, "blocks": len(payload["block_details"]),
            "biomes": len(payload.get("biomes", []))}


@app.get("/api/releases")
async def api_releases():
    if not DOWNLOADS_ENABLED:
        return {"count": 0, "releases": [], "enabled": False}
    items = release_store.list_releases()
    return {"count": len(items), "releases": items, "enabled": True}


@app.get("/api/releases/latest")
async def api_latest(platform: str = ""):
    if not DOWNLOADS_ENABLED:
        return JSONResponse({"error": "downloads_disabled",
                             "message": "The build is not public yet."}, status_code=403)
    item = release_store.latest(platform)
    if item is None:
        return JSONResponse({"error": "no_releases",
                             "message": "No build has been published yet."}, status_code=404)
    return item


@app.get("/api/admin/signups", dependencies=[Depends(apicommon.require_admin)])
async def admin_signups(limit: int = 200):
    limit = max(1, min(limit, 1000))
    with apicommon._db_lock, apicommon.db() as conn:
        rows = conn.execute(
            "SELECT id, name, email, lang, platform, message, created_at "
            "FROM signups ORDER BY id DESC LIMIT ?",
            (limit,),
        ).fetchall()
        (total,) = conn.execute("SELECT COUNT(*) FROM signups").fetchone()
    return {"count": int(total), "signups": [dict(r) for r in rows]}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=API_PORT)
