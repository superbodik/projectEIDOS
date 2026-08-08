# 🌐 public/ — сайт и лаунчер CRC EIDOS

Две отдельные штуки, обе запускаются через Python.
Проектный документ по API — [API_И_ЛАУНЧЕР.md](../API_И_ЛАУНЧЕР.md).

```
public/
  site/                    FastAPI + SQLite
    app.py                 маршруты и строки EN/UA
    auth.py                аккаунты, пароли, сессии
    updates_store.py       лента обновлений
    releases.py            манифест сборок
    wiki_data.py           вики из исходников движка
    requirements.txt
    templates/             index, wiki, updates, account
    static/shots/          ← сюда класть скриншоты (.png/.jpg)
    static/atlas.png       ← генерируется tools/atlasdump.cpp
    releases/              ← генерируется tools/make_release.py
  launcher/
    launcher.py            вход, скачивание, установка, запуск
    build_exe.py           сборка в .exe
    ui/index.html
```

## Что где лежит

| Задача | Команда |
|--------|---------|
| Запустить сайт | `cd public/site && python app.py` |
| Запустить лаунчер | `cd public/launcher && python launcher.py` |
| Собрать лаунчер в .exe | `python public/launcher/build_exe.py` |
| Обновить текстуры вики | собрать `tools/atlasdump.cpp`, запустить в корне |
| Упаковать сборку игры | `python tools/make_release.py` |
| Опубликовать changelog | `python tools/publish_update.py --version v0.7.1` |
| Снять трейлер | `python tools/make_trailer.py` |

---

## Сайт

### Установка

```bash
cd public/site
python -m venv .venv
.venv\Scripts\activate          # Windows
# source .venv/bin/activate     # Linux/macOS
pip install -r requirements.txt
```

### Запуск

```bash
python app.py
```

Откроется на `http://localhost:8005`. Английская версия — `/`, украинская — `/uk`.

Для разработки с автоперезагрузкой:

```bash
uvicorn app:app --reload --port 8005
```

### Скриншоты

Просто положи файлы в `static/shots/`. Галерея соберётся сама, порядок по имени:

```
static/shots/01-river.png
static/shots/02-caves.png
static/shots/03-mountains.png
```

Если папка пуста, раздел галереи не отрисуется — ссылка в меню тоже исчезнет.

### Настройки (`.env` рядом с `app.py`)

```ini
PORT=8005
RESEND_API_KEY=re_xxxxxxxx
RESEND_FROM=noreply@твойдомен
ADMIN_TOKEN=длинная_случайная_строка
DISCORD_URL=https://discord.gg/d7qz94Y9Bk
TELEGRAM_URL=https://t.me/CRCEIODOS
DB_PATH=beta_signups.db
```

**Без `RESEND_API_KEY` сайт работает.** Заявки принимаются и пишутся в базу, письмо просто логируется в консоль вместо отправки — удобно тестировать без ключа.

**Без `ADMIN_TOKEN`** эндпоинт со списком заявок отдаёт 404, то есть выключен. Это специально: лучше выключен, чем открыт всему интернету.

### Заявки

Все заявки лежат в SQLite (`beta_signups.db`). Посмотреть:

```bash
curl -H "Authorization: Bearer ТВОЙ_ADMIN_TOKEN" http://localhost:8005/api/admin/signups
```

Или напрямую:

```bash
sqlite3 beta_signups.db "SELECT created_at, name, email, platform FROM signups ORDER BY id DESC;"
```

### Защита

- **Rate limit**: 5 заявок с одного IP в час
- **Дедуп**: повторная заявка на тот же email не создаёт дубль, но письмо шлётся заново — если первое не дошло из-за поломки почты, человек всё равно его получит
- **robots.txt** закрывает `/api/`

⚠️ Rate limit хранится в памяти процесса: при перезапуске сбрасывается, и при нескольких воркерах у каждого свой счётчик. Для одного инстанса нормально; если пойдёшь в несколько воркеров — нужен Redis.

---

## Лаунчер

```bash
cd public/launcher
python launcher.py
```

Открывается в отдельном окне, если установлен `pywebview`:

```bash
pip install pywebview
```

Без него откроется в браузере — работает так же, просто не выглядит как приложение.

### Что умеет

- Находит собранную игру в `build/bin/Release/EidosApp.exe`
- Показывает версию и дату из `ROADMAP.md`, список изменений оттуда же
- Запускает игру и считает наигранное время (пишет в `launcher_state.json`)
- Показывает список миров из `saves/`
- Ссылки на Discord и Telegram

Если игра не собрана, кнопка запуска говорит об этом прямо, а не молча ничего не делает.

---

## Деплой сайта

Для продакшена не запускай `python app.py` напрямую — возьми uvicorn с воркерами за nginx:

```bash
uvicorn app:app --host 127.0.0.1 --port 8005
```

И не забудь HTTPS — форма собирает email-адреса.
