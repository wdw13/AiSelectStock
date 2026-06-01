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
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse
import threading

DB_WRITE_LOCK = threading.Lock()


def emit_phase(message: str):
    """
    输出阶段信息给 Qt。
    Qt 进度框可以读取这种格式：
    PHASE|正在获取股票列表...
    """
    print(f"PHASE|{message}", flush=True)


def emit_progress(current: int, total: int, code: str, status: str, message: str):
    safe_message = str(message).replace("\n", " ").replace("\r", " ").replace("|", " ")

    # 失败时不把具体报错传给 Qt，避免界面显示一大段异常
    if status == "failed":
        safe_message = "同步失败，已跳过"

    print(f"PROGRESS|{current}|{total}|{code}|{status}|{safe_message}", flush=True)

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

    emit_phase("正在同步上证指数 sh000001 ...")

    raw_df = fetch_index_daily_bars(code)

    if raw_df is None or raw_df.empty:
        emit_phase("上证指数没有新数据")
        return

    rows = normalize_index_daily_bars(code, raw_df)

    conn = get_conn()
    try:
        with DB_WRITE_LOCK:
            upsert_daily_bars(conn, rows)
            latest_date = rows[-1]["trade_date"]
            update_sync_state(conn, code, latest_date, "success", None)

        emit_phase(f"上证指数同步完成，更新 {len(rows)} 行，最新日期 {latest_date}")
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
    emit_phase("正在获取股票列表...")
    raw_df = fetch_stock_list()

    emit_phase("正在整理股票列表...")
    rows = normalize_stock_rows(raw_df)

    if not rows:
        raise RuntimeError("no stock rows after filtering")

    codes = [row["code"] for row in rows]

    emit_phase(f"正在写入 {len(rows)} 只股票到数据库...")
    conn = get_conn()
    try:
        with DB_WRITE_LOCK:
            upsert_stocks(conn, rows)
            init_sync_state_for_stocks(conn, codes)
    finally:
        conn.close()

    emit_phase("股票列表同步完成")


def next_day_str(date_str: str) -> str:
    dt = datetime.strptime(date_str, "%Y-%m-%d")
    return (dt + timedelta(days=1)).strftime("%Y%m%d")


def sync_one_stock(code: str):
    conn = get_conn()
    last_trade_date = None

    try:
        last_trade_date = get_last_trade_date(conn, code, "none")

        if last_trade_date:
            start_date = next_day_str(last_trade_date)
        else:
            start_date = "20220101"

        end_date = datetime.now().strftime("%Y%m%d")

        if start_date > end_date:
            with DB_WRITE_LOCK:
                update_sync_state(conn, code, last_trade_date, "success", "already up to date")

            return {
                "code": code,
                "status": "success",
                "message": "already up to date",
            }

        raw_df = fetch_daily_bars(code, start_date, end_date)

        if raw_df is None or raw_df.empty:
            with DB_WRITE_LOCK:
                update_sync_state(conn, code, last_trade_date, "success", "no new data")

            return {
                "code": code,
                "status": "success",
                "message": "no new data",
            }

        rows = normalize_daily_bars(code, raw_df)

        if not rows:
            with DB_WRITE_LOCK:
                update_sync_state(conn, code, last_trade_date, "failed", "normalized rows empty")

            raise RuntimeError("normalized rows empty")

        latest_date = rows[-1]["trade_date"]

        with DB_WRITE_LOCK:
            upsert_daily_bars(conn, rows)
            update_sync_state(conn, code, latest_date, "success", None)

        return {
            "code": code,
            "status": "success",
            "message": f"updated {len(rows)} rows, latest={latest_date}",
        }

    except Exception as e:
        try:
            with DB_WRITE_LOCK:
                update_sync_state(conn, code, last_trade_date, "failed", str(e))
        except Exception:
            pass

        raise

    finally:
        conn.close()

def sync_all_stocks(limit: int | None = None, offset: int = 0, workers: int = 6):
    conn = get_conn()
    try:
        codes = get_stock_codes(conn, limit=limit, offset=offset)
    finally:
        conn.close()

    total = len(codes)
    workers = max(1, int(workers))

    emit_phase(f"准备同步 {total} 只股票，线程数={workers}")

    if total <= 0:
        emit_progress(0, 0, "ALL", "success", "没有需要同步的股票")
        return

    done = 0
    failed = 0

    with ThreadPoolExecutor(max_workers=workers) as executor:
        future_map = {
            executor.submit(sync_one_stock, code): code
            for code in codes
        }

        for future in as_completed(future_map):
            code = future_map[future]
            done += 1

            try:
                result = future.result()
                emit_progress(
                    done,
                    total,
                    code,
                    result.get("status", "success"),
                    result.get("message", "success"),
                )
            except Exception as e:
                failed += 1
                emit_progress(done, total, code, "failed", str(e))

    if failed > 0:
        emit_phase(f"股票同步完成，但有 {failed} 只失败")
    else:
        emit_phase("股票同步完成，全部成功")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="同步股票列表、上证指数和日K数据")
    parser.add_argument("--workers", type=int, default=6, help="同步股票日K数据的线程数")
    parser.add_argument("--limit", type=int, default=None, help="只同步前 N 只股票，测试用")
    parser.add_argument("--offset", type=int, default=0, help="从第几只股票开始同步，测试用")

    args = parser.parse_args()

    ensure_database_ready()
    sync_stock_list()
    sync_sh_index()
    sync_all_stocks(
        limit=args.limit,
        offset=args.offset,
        workers=args.workers,
    )