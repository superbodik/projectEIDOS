"""CRC EIDOS public website - marketing pages, wiki/updates HTML, accounts.

Runs on PORT (default 8001). The JSON service API (changelog feed, wiki
data feed, releases, admin) lives in api.py on its own port - see that
file's docstring for why the split happened and what stayed here.

Auth (/api/signup, /api/auth/*) and the file download route stay on this
process because they depend on the browser's same-origin session cookie;
moving them cross-origin would need CORS-with-credentials and a shared
cookie domain for no real benefit, since the launcher (a native client,
not a browser) is free to call whichever port it likes without CORS
applying to it at all.
"""
from __future__ import annotations

import asyncio
import os
import time
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Optional

import httpx
from fastapi import FastAPI, Header, HTTPException, Request
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse, PlainTextResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from markupsafe import Markup
from pydantic import BaseModel, EmailStr, Field
from starlette.middleware.base import BaseHTTPMiddleware

import apicommon
import auth
import releases as release_store
import updates_store
import wiki_data

BASE_DIR = Path(__file__).resolve().parent

PORT = int(os.environ.get("PORT", "8001"))
RESEND_API_KEY = os.environ.get("RESEND_API_KEY", "")
RESEND_FROM = os.environ.get("RESEND_FROM", "noreply@crceidos.dev")
DISCORD_URL = os.environ.get("DISCORD_URL", "https://discord.gg/d7qz94Y9Bk")
TELEGRAM_URL = os.environ.get("TELEGRAM_URL", "https://t.me/CRCEIODOS")
COOKIE_SECURE = os.environ.get("COOKIE_SECURE", "0") == "1"
REQUIRE_LOGIN_TO_DOWNLOAD = os.environ.get("REQUIRE_LOGIN_TO_DOWNLOAD", "1") == "1"
DOWNLOADS_ENABLED = os.environ.get("DOWNLOADS_ENABLED", "0") == "1"

SMTP_HOST = os.environ.get("SMTP_HOST", "")
SMTP_PORT = int(os.environ.get("SMTP_PORT", "587"))
SMTP_USER = os.environ.get("SMTP_USER", "")
SMTP_PASSWORD = os.environ.get("SMTP_PASSWORD", "")
SMTP_FROM = os.environ.get("SMTP_FROM", RESEND_FROM)
SMTP_SSL = os.environ.get("SMTP_SSL", "0") == "1"
SMTP_STARTTLS = os.environ.get("SMTP_STARTTLS", "1") == "1"

_signup_limit = apicommon.RateLimiter(limit=5, window_seconds=60 * 60)
_register_limit = apicommon.RateLimiter(limit=8, window_seconds=60 * 60)

db = apicommon.db
_db_lock = apicommon._db_lock


def signup_exists(email: str) -> bool:
    with _db_lock, db() as conn:
        return conn.execute("SELECT 1 FROM signups WHERE email = ?", (email,)).fetchone() is not None


def insert_signup(name: str, email: str, lang: str, platform: str, message: str) -> None:
    with _db_lock, db() as conn:
        conn.execute(
            "INSERT INTO signups (name, email, lang, platform, message) VALUES (?, ?, ?, ?, ?)",
            (name, email, lang, platform, message),
        )
        conn.commit()


def count_signups() -> int:
    with _db_lock, db() as conn:
        (count,) = conn.execute("SELECT COUNT(*) FROM signups").fetchone()
        return int(count)


EMAIL_SUBJECT = {
    "en": "You are on the CRC EIDOS beta list",
    "uk": "Ви у списку бета-тесту CRC EIDOS",
}

EMAIL_BODY = {
    "en": """
    <p>Hey {name},</p>
    <p>You are on the CRC EIDOS closed beta waitlist. We will email you the moment a slot opens.</p>
    <p>EIDOS is a voxel world with real geology, rivers that flow from the mountains to the sea,
    and a ten-era progression from a flint knife to machine civilisation.</p>
    <p>Development news: <a href="{discord}">Discord</a></p>
    <p>— CRC EIDOS</p>
    """,
    "uk": """
    <p>Привіт, {name}!</p>
    <p>Ви у списку очікування закритої бети CRC EIDOS. Ми напишемо, щойно звільниться місце.</p>
    <p>EIDOS — воксельний світ зі справжньою геологією, річками, що течуть з гір у море,
    і прогресією з десяти епох: від кремневого ножа до машинної цивілізації.</p>
    <p>Новини розробки: <a href="{discord}">Discord</a></p>
    <p>— CRC EIDOS</p>
    """,
}


def send_via_smtp(to_email: str, subject: str, html: str) -> bool:
    import smtplib
    import ssl
    from email.message import EmailMessage

    msg = EmailMessage()
    msg["Subject"] = subject
    msg["From"] = SMTP_FROM or SMTP_USER
    msg["To"] = to_email
    msg.set_content("This email needs an HTML-capable client.")
    msg.add_alternative(html, subtype="html")

    try:
        if SMTP_SSL:
            server = smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT, timeout=15,
                                      context=ssl.create_default_context())
        else:
            server = smtplib.SMTP(SMTP_HOST, SMTP_PORT, timeout=15)
        with server:
            if not SMTP_SSL and SMTP_STARTTLS:
                server.starttls(context=ssl.create_default_context())
            if SMTP_USER:
                server.login(SMTP_USER, SMTP_PASSWORD)
            server.send_message(msg)
        return True
    except Exception as exc:
        print(f"[eidos-site] SMTP send to {to_email} failed: {exc}")
        return False


async def send_confirmation_email(name: str, email: str, lang: str) -> None:
    lang = lang if lang in EMAIL_BODY else "en"
    subject = EMAIL_SUBJECT[lang]
    html = EMAIL_BODY[lang].format(name=name or "there", discord=DISCORD_URL)

    if not RESEND_API_KEY and SMTP_HOST:
        if send_via_smtp(email, subject, html):
            print(f"[eidos-site] SMTP: confirmation sent to {email}")
        return

    if not RESEND_API_KEY:
        print(f"[eidos-site] no mail provider configured - would have emailed {email}:\n{html}")
        return

    payload = {"from": RESEND_FROM, "to": [email], "subject": subject, "html": html}
    headers = {
        "Authorization": f"Bearer {RESEND_API_KEY}",
        "Content-Type": "application/json",
        "User-Agent": "eidos-site/1.0",
    }

    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            resp = await client.post("https://api.resend.com/emails", json=payload, headers=headers)
            if resp.status_code >= 400:
                print(f"[eidos-site] Resend {resp.status_code} for {email}: {resp.text}")
            else:
                print(f"[eidos-site] Resend accepted mail for {email} ({resp.status_code})")
    except httpx.HTTPError as exc:
        print(f"[eidos-site] Resend request failed for {email}: {exc}")


class SignupIn(BaseModel):
    name: str = Field(min_length=1, max_length=64)
    email: EmailStr
    lang: str = Field(default="en", max_length=5)
    platform: str = Field(default="", max_length=32)
    message: str = Field(default="", max_length=500)


class RegisterIn(BaseModel):
    username: str = Field(min_length=3, max_length=24)
    email: EmailStr
    password: str = Field(min_length=8, max_length=200)
    lang: str = Field(default="en", max_length=5)


class LoginIn(BaseModel):
    identifier: str = Field(min_length=3, max_length=254)
    password: str = Field(min_length=1, max_length=200)
    source: str = Field(default="web", max_length=16)


SESSION_COOKIE = "eidos_session"


def current_user(request: Request, authorization: Optional[str] = None) -> Optional[dict]:
    token = ""
    if authorization and authorization.lower().startswith("bearer "):
        token = authorization[7:].strip()
    if not token:
        token = request.cookies.get(SESSION_COOKIE, "")
    if not token:
        return None
    with _db_lock, db() as conn:
        return auth.session_user(conn, token)


class SecurityHeaders(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next):
        started = time.time()
        response = await call_next(request)
        ms = (time.time() - started) * 1000
        apicommon.log("eidos-site",
                      f"{response.status_code} {request.method} {request.url.path} "
                      f"ip={apicommon.client_ip(request)} {ms:.0f}ms")
        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["X-Frame-Options"] = "DENY"
        response.headers["Referrer-Policy"] = "no-referrer"
        return response


@asynccontextmanager
async def lifespan(app: FastAPI):
    apicommon.startup_report("eidos-site")
    with _db_lock, db() as conn:
        apicommon.init_schema(conn)
    apicommon.log("eidos-site", f"listening on 0.0.0.0:{PORT}")
    if not RESEND_API_KEY:
        apicommon.log("eidos-site", "WARNING: RESEND_API_KEY unset - emails will be logged, not sent")
    yield


app = FastAPI(title="CRC EIDOS", lifespan=lifespan, docs_url=None, redoc_url=None)
app.add_middleware(SecurityHeaders)

templates = Jinja2Templates(directory=str(BASE_DIR / "templates"))
templates.env.filters["tojson"] = lambda v: Markup(__import__("json").dumps(v, ensure_ascii=False))
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")

STRINGS = {
    "en": {
        "lang": "en",
        "other_lang": "uk",
        "other_lang_label": "Українська",
        "nav_features": "World",
        "nav_progression": "Progression",
        "nav_gallery": "Gallery",
        "nav_beta": "Join beta",
        "hero_kicker": "Custom voxel engine . written from scratch",
        "hero_title": "CRC EIDOS",
        "hero_sub": "From primal matter to absolute control.",
        "hero_text": "A voxel world with real geology, rivers that carve their own valleys, "
                     "and a world that keeps living while you are away. Ten eras from a flint "
                     "knife to a machine civilisation that mines for you.",
        "hero_cta": "Join the closed beta",
        "hero_alt": "Free forever . no pay-to-win",
        "features_title": "What makes it different",
        "f1_t": "Rivers with real hydrology",
        "f1_d": "Rivers start in the mountains, meander, merge into tributaries and step down "
                "to the sea in chains of pools and waterfalls. The valley is carved into the "
                "terrain itself, so banks slope naturally into the water.",
        "f2_t": "Climate by latitude",
        "f2_d": "Walk north and the world gets colder: forest, taiga, snowy tundra. Temperature "
                "is a real number in degrees, built from latitude, altitude, time of day and "
                "depth underground. Caves hold a steady 11 degrees whatever the surface does.",
        "f3_t": "The world lives without you",
        "f3_d": "Grass spreads and dies under shade. Oaks drop acorns, acorns become saplings, "
                "saplings become trees. Leaves rot when their trunk is gone. None of it needs "
                "you standing there.",
        "f4_t": "Geology you can read",
        "f4_d": "Pebbles on the surface tell you what lies beneath. Soft and hard stone by "
                "biome, ore traces that lead to real veins. Learning to read the ground is a "
                "skill, not a wiki lookup.",
        "f5_t": "Survival with teeth",
        "f5_d": "Hunger, thirst, body temperature. Freeze and you take damage. Your health "
                "regeneration is capped by how balanced your diet is, so living on one food "
                "group will never make you whole.",
        "f6_t": "Automation is the whole point",
        "f6_d": "Every era frees you from the manual labour of the last. First you dig by hand. "
                "Then a river turns your mill. In the end you write scripts and the factory runs "
                "itself. Nothing disappears, it gets automated.",
        "prog_title": "Ten eras",
        "prog_sub": "Each era removes the manual work of the one before it.",
        "prog_note": "Era I is playable now. The rest is designed and being built - you will "
                     "see every step.",
        "gallery_title": "Screenshots",
        "gallery_note": "Captured in engine. Work in progress.",
        "beta_title": "Join the closed beta",
        "beta_sub": "Free forever. No paid advantages, no donation perks, nothing to buy.",
        "form_name": "Name or nickname",
        "form_email": "Email",
        "form_platform": "Platform",
        "form_message": "Anything you want to tell us (optional)",
        "form_submit": "Request a beta slot",
        "form_sending": "Sending...",
        "form_ok": "You are on the list. Check your email.",
        "form_dupe": "You were already on the list - we resent the confirmation.",
        "form_err": "Something went wrong. Try again in a moment.",
        "form_rate": "Too many requests. Try again later.",
        "form_invalid": "Please enter a name and a valid email.",
        "footer_note": "CRC EIDOS is in active development. Everything you see is subject to change.",
        "trailer_title": "Trailer",
        "nav_wiki": "Wiki",
        "nav_updates": "Updates",
        "nav_account": "Account",
        "nav_signin": "Sign in",
        "wiki_title": "Game wiki",
        "wiki_sub": "Generated straight from the engine source, so it can never drift from the build.",
        "wiki_blocks": "Blocks and textures",
        "wiki_blocks_note": "Every texture below is the real tile the game draws, exported from the engine atlas.",
        "wiki_geology": "Geology",
        "wiki_geology_note": "Five geological provinces. Which rock you hit depends on where you stand and how deep you dig.",
        "wiki_ores": "Where ore lives",
        "wiki_ores_note": "Ore is bound to its host rock. Surface pebbles tell you what is underneath.",
        "wiki_food": "Food",
        "wiki_food_note": "Right-click food in hand to eat. Health regeneration is capped by your worst nutrient group.",
        "wiki_quests": "Era I quest tree",
        "wiki_biomes": "Biomes",
        "wiki_biomes_hint": "Click a biome to see its climate, ground, and vegetation.",
        "biome_share": "Share of the world",
        "biome_temp": "Temperature",
        "biome_humidity": "Humidity",
        "biome_elevation": "Elevation",
        "biome_ground": "Ground",
        "biome_vegetation": "Vegetation",
        "wiki_controls": "Controls",
        "wiki_commands": "Console commands",
        "wiki_eras": "Coming eras",
        "updates_title": "Updates",
        "updates_sub": "Every entry is a published release. Newest first.",
        "download_title": "Download",
        "download_note": "Sign in to download a build.",
        "download_none": "No build published yet. Join the beta and we will email you.",
        "auth_register": "Create account",
        "auth_login": "Sign in",
        "auth_logout": "Sign out",
        "auth_username": "Username",
        "auth_email": "Email",
        "auth_password": "Password",
        "auth_identifier": "Username or email",
        "auth_have": "Already have an account?",
        "auth_need": "No account yet?",
        "auth_note": "This is your in-game login. Your username is shown above your character.",
        "account_title": "Your account",
        "account_member": "Member since",
        "account_lastlogin": "Last sign-in",
        "account_ingame": "In-game name",
        "beta_status_title": "Closed beta",
        "beta_status_note": "Your account is registered and reserved. The build is not "
                            "downloadable yet - we will open it here and in the launcher "
                            "when the closed beta starts. Watch Discord and Telegram.",
    },
    "uk": {
        "lang": "uk",
        "other_lang": "en",
        "other_lang_label": "English",
        "nav_features": "Світ",
        "nav_progression": "Прогресія",
        "nav_gallery": "Галерея",
        "nav_beta": "До бети",
        "hero_kicker": "Власний воксельний рушій . написаний з нуля",
        "hero_title": "CRC EIDOS",
        "hero_sub": "Від первинної матерії до абсолютного контролю.",
        "hero_text": "Воксельний світ зі справжньою геологією, річками, що самі прорізають долини, "
                     "і світом, який живе, поки вас немає. Десять епох — від кремневого ножа до "
                     "машинної цивілізації, що копає замість вас.",
        "hero_cta": "Долучитись до бети",
        "hero_alt": "Назавжди безкоштовно . без pay-to-win",
        "features_title": "Чим це відрізняється",
        "f1_t": "Річки зі справжньою гідрологією",
        "f1_d": "Річки починаються в горах, звиваються, зливаються в притоки і спускаються до "
                "моря ланцюгом плес і водоспадів. Долина вирізається в самому рельєфі, тому "
                "береги полого сходять до води.",
        "f2_t": "Клімат за широтою",
        "f2_d": "Йдіть на північ — і світ холоднішає: ліс, тайга, снігова тундра. Температура "
                "це справжнє число в градусах: широта, висота, час доби і глибина. У печерах "
                "стабільні 11 градусів, що б не робилось нагорі.",
        "f3_t": "Світ живе без вас",
        "f3_d": "Трава розповзається і гине в тіні. Дуби роняють жолуді, з жолудів ростуть "
                "саджанці, із саджанців — дерева. Листя опадає, коли стовбура вже нема. Для "
                "цього не треба стояти поруч.",
        "f4_t": "Геологію можна читати",
        "f4_d": "Камінці на поверхні підказують, що лежить під ногами. М'які й тверді породи "
                "за біомами, рудні сліди, що ведуть до справжніх жил. Читати землю — це "
                "навичка, а не пошук у вікі.",
        "f5_t": "Виживання із зубами",
        "f5_d": "Голод, спрага, температура тіла. Замерзли — отримуєте шкоду. Стеля регенерації "
                "залежить від того, наскільки збалансована ваша їжа: на одній каші повністю "
                "не одужати.",
        "f6_t": "Автоматизація — це і є суть",
        "f6_d": "Кожна епоха звільняє від ручної праці попередньої. Спершу копаєте руками. "
                "Потім річка крутить ваш млин. Наприкінці ви пишете скрипти, і завод працює сам. "
                "Ніщо не зникає — воно автоматизується.",
        "prog_title": "Десять епох",
        "prog_sub": "Кожна епоха прибирає ручну працю попередньої.",
        "prog_note": "Епоха I грається вже зараз. Решта спроєктована і будується — ви побачите "
                     "кожен крок.",
        "gallery_title": "Скриншоти",
        "gallery_note": "Знято в рушії. Робота триває.",
        "beta_title": "Долучитись до закритої бети",
        "beta_sub": "Назавжди безкоштовно. Без платних переваг, без донат-привілеїв, купувати нічого не треба.",
        "form_name": "Ім'я або нікнейм",
        "form_email": "Email",
        "form_platform": "Платформа",
        "form_message": "Що хочете нам сказати (необов'язково)",
        "form_submit": "Подати заявку",
        "form_sending": "Надсилаємо...",
        "form_ok": "Ви у списку. Перевірте пошту.",
        "form_dupe": "Ви вже були у списку — ми надіслали підтвердження ще раз.",
        "form_err": "Щось пішло не так. Спробуйте за хвилину.",
        "form_rate": "Забагато запитів. Спробуйте пізніше.",
        "form_invalid": "Вкажіть ім'я та коректний email.",
        "footer_note": "CRC EIDOS в активній розробці. Усе, що ви бачите, може змінитись.",
        "trailer_title": "Трейлер",
        "nav_wiki": "Вікі",
        "nav_updates": "Оновлення",
        "nav_account": "Акаунт",
        "nav_signin": "Увійти",
        "wiki_title": "Вікі гри",
        "wiki_sub": "Генерується просто з вихідників рушія, тому не може розійтися зі збіркою.",
        "wiki_blocks": "Блоки й текстури",
        "wiki_blocks_note": "Кожна текстура нижче — це справжній тайл, який малює гра, вивантажений з атласу рушія.",
        "wiki_geology": "Геологія",
        "wiki_geology_note": "П'ять геологічних провінцій. Яка порода трапиться — залежить від того, де ви стоїте і як глибоко копаєте.",
        "wiki_ores": "Де шукати руду",
        "wiki_ores_note": "Руда прив'язана до вмісної породи. Камінці на поверхні підказують, що під ногами.",
        "wiki_food": "Їжа",
        "wiki_food_note": "Правий клік — з'їсти те, що в руці. Стеля регенерації залежить від найгіршої групи нутрієнтів.",
        "wiki_quests": "Дерево квестів Епохи I",
        "wiki_biomes": "Біоми",
        "wiki_biomes_hint": "Натисніть на біом, щоб побачити клімат, ґрунт і рослинність.",
        "biome_share": "Частка світу",
        "biome_temp": "Температура",
        "biome_humidity": "Вологість",
        "biome_elevation": "Висота",
        "biome_ground": "Ґрунт",
        "biome_vegetation": "Рослинність",
        "wiki_controls": "Керування",
        "wiki_commands": "Команди консолі",
        "wiki_eras": "Майбутні епохи",
        "updates_title": "Оновлення",
        "updates_sub": "Кожен запис — опублікований реліз. Найновіші згори.",
        "download_title": "Завантажити",
        "download_note": "Увійдіть, щоб завантажити збірку.",
        "download_none": "Збірку ще не опубліковано. Долучайтесь до бети — ми напишемо.",
        "auth_register": "Створити акаунт",
        "auth_login": "Увійти",
        "auth_logout": "Вийти",
        "auth_username": "Нікнейм",
        "auth_email": "Email",
        "auth_password": "Пароль",
        "auth_identifier": "Нікнейм або email",
        "auth_have": "Вже маєте акаунт?",
        "auth_need": "Ще немає акаунта?",
        "auth_note": "Це ваш логін у грі. Нікнейм показується над персонажем.",
        "account_title": "Ваш акаунт",
        "account_member": "З нами з",
        "account_lastlogin": "Останній вхід",
        "account_ingame": "Ім'я у грі",
        "beta_status_title": "Закрита бета",
        "beta_status_note": "Ваш акаунт зареєстровано і місце закріплено. Збірку ще не можна "
                            "завантажити — ми відкриємо її тут і в лаунчері, коли стартує "
                            "закрита бета. Стежте за Discord і Telegram.",
    },
}

ERAS = [
    ("I", "Primal Chaos", "Первинний Хаос", "flint, fire, physics", "кремінь, вогонь, фізика", True),
    ("II", "Clay & Casting", "Кераміка та Литво", "pit kiln, copper", "ямний обжиг, мідь", False),
    ("III", "Kinematics", "Кінематика", "bronze, water wheel", "бронза, водяне колесо", False),
    ("IV", "Thermodynamics", "Термодинаміка", "bloomery, steel", "сиродутна піч, сталь", False),
    ("V", "Infrastructure", "Інфраструктура", "gunpowder, windmills", "порох, вітряки", False),
    ("VI", "Steam Engine", "Паровий Рушій", "conveyors, drills", "конвеєри, бури", False),
    ("VII", "EIDOS Grid", "Магістралі EIDOS", "power, logic", "енергія, логіка", False),
    ("VIII", "Information", "Інформаційна", "Lua scripting, drones", "скрипти Lua, дрони", False),
    ("IX", "Orbital Vector", "Орбітальний Вектор", "rockets, planets", "ракети, планети", False),
    ("X", "EIDOS Absolute", "Абсолют EIDOS", "AI core, terraforming", "ІІ-ядро, терраформінг", False),
]


def trailer_files() -> tuple[str, str, str]:
    video_dir = BASE_DIR / "static" / "video"
    if not video_dir.is_dir():
        return "", "", ""
    mp4 = next((p.name for p in sorted(video_dir.glob("*.mp4"))), "")
    if not mp4:
        return "", "", ""
    stem = mp4[:-4]
    webm = f"{stem}.webm" if (video_dir / f"{stem}.webm").is_file() else ""
    poster = f"{stem}.jpg" if (video_dir / f"{stem}.jpg").is_file() else ""
    return mp4, webm, poster


def gallery_images() -> list[str]:
    shots = BASE_DIR / "static" / "shots"
    if not shots.is_dir():
        return []
    names = [p.name for p in shots.iterdir()
             if p.suffix.lower() in {".png", ".jpg", ".jpeg", ".webp"}]
    return sorted(names)


def render(request: Request, lang: str) -> HTMLResponse:
    t = STRINGS[lang]
    mp4, webm, poster = trailer_files()
    eras = [
        {
            "num": num,
            "name": en_name if lang == "en" else uk_name,
            "hint": en_hint if lang == "en" else uk_hint,
            "playable": playable,
        }
        for num, en_name, uk_name, en_hint, uk_hint, playable in ERAS
    ]
    return templates.TemplateResponse(
        request,
        "index.html",
        {
            "t": t,
            "eras": eras,
            "shots": gallery_images(),
            "discord": DISCORD_URL,
            "telegram": TELEGRAM_URL,
            "user": current_user(request),
            "trailer": mp4,
            "trailer_webm": webm,
            "trailer_poster": poster,
        },
    )


def atlas_map() -> dict:
    path = BASE_DIR / "static" / "atlas_map.json"
    if not path.exists():
        return {"grid": 16, "tile": 16, "blocks": {}}
    try:
        import json
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {"grid": 16, "tile": 16, "blocks": {}}


BLOCK_GROUPS = [
    ("Rock", range(16, 46)),
    ("Ore", range(50, 67)),
    ("Ground", range(5, 16)),
    ("Wood and leaves", range(100, 110)),
    ("Plants", list(range(110, 130))),
    ("Pebbles", range(130, 150)),
    ("Food and forage", range(150, 160)),
    ("Liquids and ice", [1, 2, 3, 4, 120, 121, 122]),
]


def wiki_blocks() -> list[dict]:
    amap = atlas_map()
    blocks = amap.get("blocks", {})
    grid = amap.get("grid", 16)
    tile = amap.get("tile", 16)
    sheet = grid * tile

    groups = []
    used: set[int] = set()
    for label, ids in BLOCK_GROUPS:
        items = []
        for bid in ids:
            entry = blocks.get(str(bid))
            if not entry or bid in used:
                continue
            used.add(bid)
            col, row = entry.get("side", [0, 0])
            items.append({
                "id": bid,
                "name": entry.get("name", str(bid)),
                "x": col * tile,
                "y": row * tile,
                "sheet": sheet,
            })
        if items:
            groups.append({"label": label, "blocks": items})
    return groups


def biome_shots() -> dict:
    d = BASE_DIR / "static" / "biomes"
    if not d.is_dir():
        return {}
    return {p.stem: p.name for p in d.iterdir() if p.suffix.lower() == ".png"}


def base_context(request: Request, lang: str) -> dict:
    return {
        "t": STRINGS[lang],
        "discord": DISCORD_URL,
        "telegram": TELEGRAM_URL,
        "user": current_user(request),
        "home": "/" if lang == "en" else "/uk",
        "prefix": "" if lang == "en" else "/uk",
        "other_prefix": "/uk" if lang == "en" else "",
    }


def latest_release_stamp() -> tuple[str, str]:
    """Version/date shown in page headers - from the published-updates
    table, not ROADMAP.md (the deployed site container has no reason to
    ship the engine's source tree, so parsing it there returns nothing)."""
    with apicommon._db_lock, apicommon.db() as conn:
        row = updates_store.latest(conn, channel="beta")
    if not row:
        return "—", "—"
    return row["version"], row["published_at"]


def render_wiki(request: Request, lang: str) -> HTMLResponse:
    data = wiki_data.get()
    version, updated = latest_release_stamp()
    data = {**data, "version": version, "updated": updated}
    ctx = base_context(request, lang)
    ctx.update({
        "wiki": data,
        "groups": wiki_blocks(),
        "biome_shots": biome_shots(),
        "controls": [(k, en if lang == "en" else uk) for k, en, uk in wiki_data.CONTROLS],
        "commands": [(c, en if lang == "en" else uk) for c, en, uk in wiki_data.COMMANDS],
        "page": "wiki",
    })
    return templates.TemplateResponse(request, "wiki.html", ctx)


UPDATE_LABEL_EN = {"added": "Added", "changed": "Changed", "fixed": "Fixed", "known": "Known issue"}
UPDATE_LABEL_UK = {"added": "Додано", "changed": "Змінено", "fixed": "Виправлено", "known": "Відома проблема"}


def render_updates(request: Request, lang: str) -> HTMLResponse:
    # Pulled straight from the published-updates table (the same data
    # /api/updates serves) instead of re-parsing ROADMAP.md at request
    # time - the deployed site container has no reason to ship the
    # engine's whole source tree just so this page can grep it.
    labels = UPDATE_LABEL_EN if lang == "en" else UPDATE_LABEL_UK
    with apicommon._db_lock, apicommon.db() as conn:
        rows = updates_store.listing(conn, limit=50, channel="beta")

    entries = []
    for row in rows:
        details = []
        for section in ("added", "changed", "fixed", "known"):
            for line in row[section]:
                details.append({"cat": section, "label": labels[section], "text": line})
        entries.append({
            "date": row["published_at"],
            "icon": "",
            "title": f"{row['version']} — {row['title']}",
            "summary": row["summary"],
            "details": details,
        })

    version, updated = latest_release_stamp()
    ctx = base_context(request, lang)
    ctx.update({
        "entries": entries,
        "version": version,
        "updated": updated,
        "page": "updates",
    })
    return templates.TemplateResponse(request, "updates.html", ctx)


def render_account(request: Request, lang: str) -> HTMLResponse:
    ctx = base_context(request, lang)
    ctx.update({
        "releases": release_store.list_releases() if DOWNLOADS_ENABLED else [],
        "downloads_enabled": DOWNLOADS_ENABLED,
        "page": "account",
    })
    return templates.TemplateResponse(request, "account.html", ctx)


@app.get("/", response_class=HTMLResponse)
async def index_en(request: Request):
    return render(request, "en")


@app.get("/uk", response_class=HTMLResponse)
async def index_uk(request: Request):
    return render(request, "uk")


@app.get("/robots.txt", response_class=PlainTextResponse)
async def robots():
    return "User-agent: *\nDisallow: /api/\nAllow: /\n"


@app.get("/health")
async def health():
    return {"status": "ok", "signups": count_signups()}


@app.post("/api/signup")
async def signup(payload: SignupIn, request: Request):
    ip = apicommon.client_ip(request)
    if _signup_limit.hit(ip):
        return JSONResponse({"error": "rate_limited"}, status_code=429)

    name = payload.name.strip()
    email = payload.email.lower().strip()
    lang = payload.lang if payload.lang in STRINGS else "en"
    platform = payload.platform.strip()
    message = payload.message.strip()

    if not name:
        return JSONResponse({"error": "invalid"}, status_code=422)

    if signup_exists(email):
        await send_confirmation_email(name, email, lang)
        return JSONResponse({"ok": True, "already_signed_up": True}, status_code=200)

    insert_signup(name, email, lang, platform, message)
    await send_confirmation_email(name, email, lang)
    return JSONResponse({"ok": True}, status_code=201)


@app.post("/api/auth/register")
async def api_register(payload: RegisterIn, request: Request):
    ip = apicommon.client_ip(request)
    if _register_limit.hit(ip):
        return JSONResponse({"error": "rate_limited",
                             "message": "Too many requests. Try again later."}, status_code=429)
    try:
        with _db_lock, db() as conn:
            user = auth.create_user(conn, payload.username, str(payload.email),
                                    payload.password, payload.lang)
            token = auth.create_session(conn, user["id"], "web")
    except auth.AuthError as exc:
        return JSONResponse({"error": exc.code, "message": exc.message}, status_code=exc.status)

    resp = JSONResponse({"ok": True, "user": user, "token": token}, status_code=201)
    resp.set_cookie(SESSION_COOKIE, token, max_age=auth.SESSION_DAYS * 86400,
                    httponly=True, samesite="lax", secure=COOKIE_SECURE, path="/")
    return resp


@app.post("/api/auth/login")
async def api_login(payload: LoginIn, request: Request):
    ip = apicommon.client_ip(request)
    if auth.login_rate_limited(ip):
        return JSONResponse({"error": "rate_limited",
                             "message": "Too many attempts. Wait 15 minutes."}, status_code=429)
    source = "launcher" if payload.source == "launcher" else "web"
    try:
        with _db_lock, db() as conn:
            token, user = auth.login(conn, payload.identifier, payload.password, source)
    except auth.AuthError as exc:
        return JSONResponse({"error": exc.code, "message": exc.message}, status_code=exc.status)

    auth.clear_login_attempts(ip)
    resp = JSONResponse({"ok": True, "user": user, "token": token})
    resp.set_cookie(SESSION_COOKIE, token, max_age=auth.SESSION_DAYS * 86400,
                    httponly=True, samesite="lax", secure=COOKIE_SECURE, path="/")
    return resp


@app.post("/api/auth/logout")
async def api_logout(request: Request, authorization: Optional[str] = Header(default=None)):
    token = ""
    if authorization and authorization.lower().startswith("bearer "):
        token = authorization[7:].strip()
    if not token:
        token = request.cookies.get(SESSION_COOKIE, "")
    with _db_lock, db() as conn:
        auth.logout(conn, token)
    resp = JSONResponse({"ok": True})
    resp.delete_cookie(SESSION_COOKIE, path="/")
    return resp


@app.get("/api/auth/me")
async def api_me(request: Request, authorization: Optional[str] = Header(default=None)):
    user = current_user(request, authorization)
    if user is None:
        return JSONResponse({"error": "unauthenticated"}, status_code=401)
    return {"ok": True, "user": user}


DOWNLOADS_OFF = {
    "error": "downloads_disabled",
    "message": "The build is not public yet. Your account is registered - "
               "we will open downloads when the closed beta starts.",
}


@app.get("/download/{filename}")
async def download(filename: str, request: Request,
                   authorization: Optional[str] = Header(default=None)):
    if not DOWNLOADS_ENABLED:
        raise HTTPException(status_code=403, detail=DOWNLOADS_OFF["message"])

    if REQUIRE_LOGIN_TO_DOWNLOAD and current_user(request, authorization) is None:
        raise HTTPException(status_code=401, detail="Sign in to download the build")

    path = release_store.safe_file(filename)
    if path is None:
        raise HTTPException(status_code=404, detail="not found")

    return FileResponse(path, filename=path.name, media_type="application/octet-stream")


@app.get("/wiki", response_class=HTMLResponse)
async def wiki_en(request: Request):
    return render_wiki(request, "en")


@app.get("/uk/wiki", response_class=HTMLResponse)
async def wiki_uk(request: Request):
    return render_wiki(request, "uk")


@app.get("/updates", response_class=HTMLResponse)
async def updates_en(request: Request):
    return render_updates(request, "en")


@app.get("/uk/updates", response_class=HTMLResponse)
async def updates_uk(request: Request):
    return render_updates(request, "uk")


@app.get("/account", response_class=HTMLResponse)
async def account_en(request: Request):
    return render_account(request, "en")


@app.get("/uk/account", response_class=HTMLResponse)
async def account_uk(request: Request):
    return render_account(request, "uk")


async def _run_both() -> None:
    """Single-process launch for hosting panels that only allow one process
    per app slot. The site (this app, PORT) and the API (api.py, API_PORT)
    stay two separate FastAPI apps in two separate files - only the OS
    process is shared, running both uvicorn servers on the same event loop.
    This is uvicorn's own documented pattern for multiple servers in one
    process (see Server.serve() / capture_signals())."""
    import uvicorn

    import api as api_module

    site_server = uvicorn.Server(uvicorn.Config(app, host="0.0.0.0", port=PORT))
    api_server = uvicorn.Server(uvicorn.Config(api_module.app, host="0.0.0.0",
                                               port=api_module.API_PORT))
    apicommon.log("eidos-site", f"single-process mode: site on :{PORT}, "
                  f"api on :{api_module.API_PORT}")
    await asyncio.gather(site_server.serve(), api_server.serve())


if __name__ == "__main__":
    asyncio.run(_run_both())
