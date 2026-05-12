from ak_client import fetch_stock_list, fetch_daily_bars, fetch_index_daily_bars
from pinyin_util import to_pinyin, to_pinyin_abbr
from db import (
    get_conn,
    init_db,
    upsert_stocks,
    init_sync_state_for_stocks,
    get_last_trade_date,
    upsert_daily_bars,
    update_sync_state,
    get_stock_codes,
)
from datetime import datetime, timedelta
import time


def normalize_code(raw_code: str) -> str:
    code = str(raw_code).strip().lower()
    if code.startswith(("sh", "sz", "bj")):
        code = code[2:]
    return code


def is_normal_main_a(code: str) -> bool:
    """
    只保留沪深主板：
    - 上海主板：600 / 601 / 603 / 605
    - 深圳主板：000 / 001

    不包含：
    - 创业板：300
    - 科创板：688
    - 中小板：002
    - 北交所：8xx / 4xx
    """
    if not code:
        return False

    return code.startswith((
        "600", "601", "603", "605",
        "000", "001"
    ))

def is_st_stock(name: str) -> bool:
    """
    过滤 ST / *ST / S*ST 股票。
    这些股票涨跌幅通常是 5%，不放入数据库。
    """
    if not name:
        return False

    name = str(name).upper().strip()

    return "ST" in name

# def normalize_daily_bars(code: str, raw_df):
#     rows = []

#     for _, row in raw_df.iterrows():
#         trade_date = str(row["日期"]).strip()

#         rows.append({
#             "code": code,
#             "trade_date": trade_date,
#             "open": float(row["开盘"]),
#             "high": float(row["最高"]),
#             "low": float(row["最低"]),
#             "close": float(row["收盘"]),
#             "volume": float(row["成交量"]) if row["成交量"] is not None else None,
#             "amount": float(row["成交额"]) if row["成交额"] is not None else None,
#             "pct_chg": float(row["涨跌幅"]) if row["涨跌幅"] is not None else None,
#             "turnover": float(row["换手率"]) if row["换手率"] is not None else None,
#             "adj_type": "none",
#         })

#     return rows
def safe_float(value):
    if value is None:
        return None

    try:
        # pandas 里面的 NaN 特殊处理
        if value != value:
            return None
        return float(value)
    except Exception:
        return None


def normalize_daily_bars(code: str, raw_df):
    """
    现在 fetch_daily_bars 已经把三种接口统一成这些字段：
    code, trade_date, open, high, low, close, volume, amount, pct_chg, turnover, source
    """
    rows = []

    for _, row in raw_df.iterrows():
        trade_date = str(row["trade_date"]).strip()

        rows.append({
            "code": code,
            "trade_date": trade_date,
            "open": safe_float(row["open"]),
            "high": safe_float(row["high"]),
            "low": safe_float(row["low"]),
            "close": safe_float(row["close"]),
            "volume": safe_float(row["volume"]),
            "amount": safe_float(row["amount"]),
            "pct_chg": safe_float(row["pct_chg"]),
            "turnover": safe_float(row["turnover"]),
            "adj_type": "none",
        })

    return rows

def normalize_index_daily_bars(code: str, raw_df):
    rows = []

    for _, row in raw_df.iterrows():
        trade_date = str(row["date"]).strip()

        rows.append({
            "code": code,
            "trade_date": trade_date,
            "open": float(row["open"]),
            "high": float(row["high"]),
            "low": float(row["low"]),
            "close": float(row["close"]),
            "volume": float(row["volume"]) if row["volume"] is not None else None,
            "amount": None,
            "pct_chg": None,
            "turnover": None,
            "adj_type": "none",
        })

    return rows

def sync_sh_index():
    code = "sh000001"

    print("[index] syncing 上证指数 sh000001 ...")

    raw_df = fetch_index_daily_bars(code)

    if raw_df is None or raw_df.empty:
        print("[index] no data")
        return

    rows = normalize_index_daily_bars(code, raw_df)

    conn = get_conn()
    try:
        upsert_daily_bars(conn, rows)
        latest_date = rows[-1]["trade_date"]
        update_sync_state(conn, code, latest_date, "success", None)
        print(f"[index] inserted/updated {len(rows)} rows, latest_date={latest_date}")
    finally:
        conn.close()

def detect_market(raw_code: str, code: str) -> str:
    raw_code = str(raw_code).strip().lower()

    if raw_code.startswith("sh"):
        return "SH"
    if raw_code.startswith("sz"):
        return "SZ"

    # 兜底
    if code.startswith(("600", "601", "603", "605")):
        return "SH"
    if code.startswith(("000", "001")):
        return "SZ"

    return ""


def normalize_stock_rows(raw_df):
    rows = []

    for _, row in raw_df.iterrows():
        raw_code = str(row["代码"]).strip()
        name = str(row["名称"]).strip()

        if not raw_code or not name:
            continue

        code = normalize_code(raw_code)

        if not is_normal_main_a(code):
            continue

        if is_st_stock(name):
            continue

        rows.append({
            "code": code,
            "name": name,
            "market": detect_market(raw_code, code),
            "board": "main",
            "is_normal_a": 1,
            "listed_date": None,
            "status": "active",
            "pinyin": to_pinyin(name),
            "pinyin_abbr": to_pinyin_abbr(name),
        })

    return rows

def ensure_database_ready():
    conn = get_conn()
    try:
        init_db(conn)
    finally:
        conn.close()

def sync_stock_list():
    print("[1/4] fetching stock list...")
    raw_df = fetch_stock_list()

    print("[2/4] normalizing stock rows...")
    rows = normalize_stock_rows(raw_df)

    if not rows:
        raise RuntimeError("no stock rows after filtering")

    codes = [row["code"] for row in rows]

    print(f"[3/4] writing {len(rows)} stocks into database...")
    conn = get_conn()
    try:
        upsert_stocks(conn, rows)
        init_sync_state_for_stocks(conn, codes)
    finally:
        conn.close()

    print("[4/4] stock list sync done.")


def next_day_str(date_str: str) -> str:
    dt = datetime.strptime(date_str, "%Y-%m-%d")
    return (dt + timedelta(days=1)).strftime("%Y%m%d")


def sync_one_stock(code: str):
    conn = get_conn()
    try:
        last_trade_date = get_last_trade_date(conn, code, "none")

        if last_trade_date:
            start_date = next_day_str(last_trade_date)
        else:
            start_date = "20220101"

        end_date = datetime.now().strftime("%Y%m%d")

        print(f"[sync_one] code={code}, start_date={start_date}, end_date={end_date}")

        if start_date > end_date:
            update_sync_state(conn, code, last_trade_date, "success", "already up to date")
            print("[sync_one] already up to date")
            return

        raw_df = fetch_daily_bars(code, start_date, end_date)

        if raw_df is None or raw_df.empty:
            update_sync_state(conn, code, last_trade_date, "success", "no new data")
            print("[sync_one] no new data")
            return

        rows = normalize_daily_bars(code, raw_df)

        if not rows:
            update_sync_state(conn, code, last_trade_date, "failed", "normalized rows empty")
            raise RuntimeError("normalized rows empty")

        upsert_daily_bars(conn, rows)

        latest_date = rows[-1]["trade_date"]
        update_sync_state(conn, code, latest_date, "success", None)

        print(f"[sync_one] inserted/updated {len(rows)} rows, latest_date={latest_date}")

    except Exception as e:
        update_sync_state(conn, code, last_trade_date, "failed", str(e))
        raise
    finally:
        conn.close()

def sync_all_stocks(limit: int | None = None, offset: int = 0):
    conn = get_conn()
    try:
        codes = get_stock_codes(conn, limit=limit, offset=offset)
    finally:
        conn.close()

    total = len(codes)
    print(f"[sync_all] total codes = {total}")

    for i, code in enumerate(codes, start=1):
        print(f"[{i}/{total}] syncing {code} ...")
        try:
            sync_one_stock(code)
        except Exception as e:
            print(f"[ERROR] code={code}, error={e}")

        time.sleep(0.3)


if __name__ == "__main__":
    ensure_database_ready()
    sync_stock_list()
    sync_sh_index()
    sync_all_stocks()
    # sync_one_stock("000001")
    # sync_all_stocks(limit=20, offset=0)