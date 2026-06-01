from pathlib import Path
from datetime import datetime
import sqlite3


def get_project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def get_default_db_path() -> Path:
    return get_project_root() / "data" / "database.db"


def ensure_parent_dir(db_path: Path) -> None:
    db_path.parent.mkdir(parents=True, exist_ok=True)


def get_conn(db_path: Path | str | None = None) -> sqlite3.Connection:
    if db_path is None:
        db_path = get_default_db_path()

    db_path = Path(db_path)
    ensure_parent_dir(db_path)

    conn = sqlite3.connect(str(db_path), timeout=60.0)
    conn.row_factory = sqlite3.Row

    # 常用优化参数
    conn.execute("PRAGMA journal_mode=WAL;")
    conn.execute("PRAGMA synchronous=NORMAL;")
    conn.execute("PRAGMA foreign_keys=ON;")
    conn.execute("PRAGMA busy_timeout=60000;")

    return conn


def init_db(conn: sqlite3.Connection) -> None:
    conn.executescript(
        """
        CREATE TABLE IF NOT EXISTS stocks (
            code TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            market TEXT NOT NULL,
            board TEXT NOT NULL,
            is_normal_a INTEGER NOT NULL,
            listed_date TEXT,
            status TEXT DEFAULT 'active',
            pinyin TEXT,
            pinyin_abbr TEXT,
            updated_at TEXT
        );

        CREATE TABLE IF NOT EXISTS daily_bars (
            code TEXT NOT NULL,
            trade_date TEXT NOT NULL,
            open REAL NOT NULL,
            high REAL NOT NULL,
            low REAL NOT NULL,
            close REAL NOT NULL,
            volume REAL,
            amount REAL,
            pct_chg REAL,
            turnover REAL,
            adj_type TEXT NOT NULL,
            updated_at TEXT,
            PRIMARY KEY (code, trade_date, adj_type)
        );

        CREATE TABLE IF NOT EXISTS sync_state (
            code TEXT PRIMARY KEY,
            last_trade_date TEXT,
            last_sync_time TEXT,
            sync_status TEXT,
            message TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_daily_code_date
        ON daily_bars(code, trade_date);

        CREATE INDEX IF NOT EXISTS idx_daily_date
        ON daily_bars(trade_date);
        """
    )
    conn.commit()

def upsert_stocks(conn: sqlite3.Connection, rows: list[dict]) -> None:
    if not rows:
        return

    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    sql = """
    INSERT INTO stocks (
        code, name, market, board, is_normal_a,
        listed_date, status, pinyin, pinyin_abbr, updated_at
    )
    VALUES (
        :code, :name, :market, :board, :is_normal_a,
        :listed_date, :status, :pinyin, :pinyin_abbr, :updated_at
    )
    ON CONFLICT(code) DO UPDATE SET
        name = excluded.name,
        market = excluded.market,
        board = excluded.board,
        is_normal_a = excluded.is_normal_a,
        listed_date = excluded.listed_date,
        status = excluded.status,
        pinyin = excluded.pinyin,
        pinyin_abbr = excluded.pinyin_abbr,
        updated_at = excluded.updated_at
    """

    payload = []
    for row in rows:
        item = dict(row)
        item["updated_at"] = now_str
        payload.append(item)

    conn.executemany(sql, payload)
    conn.commit()

def init_sync_state_for_stocks(conn: sqlite3.Connection, codes: list[str]) -> None:
    if not codes:
        return

    sql = """
    INSERT OR IGNORE INTO sync_state (
        code, last_trade_date, last_sync_time, sync_status, message
    )
    VALUES (?, NULL, NULL, 'init', NULL)
    """

    conn.executemany(sql, [(code,) for code in codes])
    conn.commit()

def get_last_trade_date(conn: sqlite3.Connection, code: str, adj_type: str = "none") -> str | None:
    row = conn.execute(
        """
        SELECT MAX(trade_date) AS last_trade_date
        FROM daily_bars
        WHERE code = ? AND adj_type = ?
        """,
        (code, adj_type)
    ).fetchone()

    if row is None:
        return None

    return row["last_trade_date"]

def upsert_daily_bars(conn: sqlite3.Connection, rows: list[dict]) -> None:
    if not rows:
        return

    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    sql = """
    INSERT INTO daily_bars (
        code, trade_date, open, high, low, close,
        volume, amount, pct_chg, turnover, adj_type, updated_at
    )
    VALUES (
        :code, :trade_date, :open, :high, :low, :close,
        :volume, :amount, :pct_chg, :turnover, :adj_type, :updated_at
    )
    ON CONFLICT(code, trade_date, adj_type) DO UPDATE SET
        open = excluded.open,
        high = excluded.high,
        low = excluded.low,
        close = excluded.close,
        volume = excluded.volume,
        amount = excluded.amount,
        pct_chg = excluded.pct_chg,
        turnover = excluded.turnover,
        updated_at = excluded.updated_at
    """

    payload = []
    for row in rows:
        item = dict(row)
        item["updated_at"] = now_str
        payload.append(item)

    conn.executemany(sql, payload)
    conn.commit()

def update_sync_state(
    conn: sqlite3.Connection,
    code: str,
    last_trade_date: str | None,
    sync_status: str,
    message: str | None = None
) -> None:
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    conn.execute(
        """
        INSERT INTO sync_state (
            code, last_trade_date, last_sync_time, sync_status, message
        )
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(code) DO UPDATE SET
            last_trade_date = excluded.last_trade_date,
            last_sync_time = excluded.last_sync_time,
            sync_status = excluded.sync_status,
            message = excluded.message
        """,
        (code, last_trade_date, now_str, sync_status, message)
    )
    conn.commit()

def get_stock_codes(conn, limit: int | None = None, offset: int = 0) -> list[str]:
    sql = """
    SELECT code
    FROM stocks
    WHERE is_normal_a = 1
    ORDER BY code ASC
    """
    params = []

    if limit is not None:
        sql += " LIMIT ? OFFSET ?"
        params.extend([limit, offset])

    rows = conn.execute(sql, params).fetchall()
    return [row["code"] for row in rows]
