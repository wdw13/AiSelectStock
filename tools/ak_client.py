import akshare as ak
import pandas as pd


def fetch_stock_list() -> pd.DataFrame:
    df = ak.stock_zh_a_spot()
    if df is None or df.empty:
        raise RuntimeError("fetch_stock_list failed: empty result")
    return df

def fetch_daily_bars(code: str, start_date: str, end_date: str) -> pd.DataFrame:
    df = ak.stock_zh_a_hist(
        symbol=code,
        period="daily",
        start_date=start_date,
        end_date=end_date,
        adjust=""
    )
    if df is None:
        raise RuntimeError(f"fetch_daily_bars failed: code={code}, result is None")
    return df

def fetch_index_daily_bars(symbol: str) -> pd.DataFrame:
    df = ak.stock_zh_index_daily(symbol=symbol)
    if df is None:
        raise RuntimeError(f"fetch_index_daily_bars failed: symbol={symbol}, result is None")
    return df