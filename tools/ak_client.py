# import akshare as ak
# import pandas as pd


# def fetch_stock_list() -> pd.DataFrame:
#     df = ak.stock_zh_a_spot()
#     if df is None or df.empty:
#         raise RuntimeError("fetch_stock_list failed: empty result")
#     return df

# def fetch_daily_bars(code: str, start_date: str, end_date: str) -> pd.DataFrame:
#     df = ak.stock_zh_a_hist(
#         symbol=code,
#         period="daily",
#         start_date=start_date,
#         end_date=end_date,
#         adjust=""
#     )
#     if df is None:
#         raise RuntimeError(f"fetch_daily_bars failed: code={code}, result is None")
#     return df

# def fetch_index_daily_bars(symbol: str) -> pd.DataFrame:
#     df = ak.stock_zh_index_daily(symbol=symbol)
#     if df is None:
#         raise RuntimeError(f"fetch_index_daily_bars failed: symbol={symbol}, result is None")
#     return df
import time
import random
import akshare as ak
import pandas as pd


def normalize_code(raw_code: str) -> str:
    code = str(raw_code).strip().lower()
    if code.startswith(("sh", "sz", "bj")):
        code = code[2:]
    return code


def code_with_market(code: str) -> str:
    """
    stock_zh_a_daily / stock_zh_a_hist_tx 需要：
    上海：sh600000
    深圳：sz000001
    """
    code = normalize_code(code)

    if code.startswith(("600", "601", "603", "605", "688")):
        return "sh" + code

    if code.startswith(("000", "001", "002", "003", "300")):
        return "sz" + code

    if code.startswith(("8", "4")):
        return "bj" + code

    return code


def fetch_stock_list() -> pd.DataFrame:
    """
    推荐用东财实时列表。
    这个接口返回沪深京 A 股所有股票，字段包括：代码、名称等。
    """
    df = ak.stock_zh_a_spot()
    if df is None or df.empty:
        raise RuntimeError("fetch_stock_list failed: empty result")
    return df


def _sleep_before_retry():
    time.sleep(random.uniform(0.8, 2.0))


def _standardize_from_hist(code: str, df: pd.DataFrame) -> pd.DataFrame:
    """
    stock_zh_a_hist 返回中文列：
    日期 股票代码 开盘 收盘 最高 最低 成交量 成交额 振幅 涨跌幅 涨跌额 换手率
    """
    if df is None or df.empty:
        return pd.DataFrame()

    out = pd.DataFrame()
    out["trade_date"] = df["日期"].astype(str)
    out["open"] = pd.to_numeric(df["开盘"], errors="coerce")
    out["high"] = pd.to_numeric(df["最高"], errors="coerce")
    out["low"] = pd.to_numeric(df["最低"], errors="coerce")
    out["close"] = pd.to_numeric(df["收盘"], errors="coerce")
    out["volume"] = pd.to_numeric(df["成交量"], errors="coerce")
    out["amount"] = pd.to_numeric(df["成交额"], errors="coerce")
    out["pct_chg"] = pd.to_numeric(df["涨跌幅"], errors="coerce")
    out["turnover"] = pd.to_numeric(df["换手率"], errors="coerce")
    out["source"] = "hist_em"
    out["code"] = normalize_code(code)

    return out.dropna(subset=["trade_date", "open", "high", "low", "close"])


def _standardize_from_daily(code: str, df: pd.DataFrame) -> pd.DataFrame:
    """
    stock_zh_a_daily 返回英文列：
    date open high low close volume amount outstanding_share turnover

    注意：
    - volume 单位是股
    - 原来 stock_zh_a_hist 的成交量单位是手
    - 所以这里统一除以 100，转成手
    - turnover 是比例，例如 0.01 表示 1%，这里乘以 100，和 stock_zh_a_hist 保持一致
    """
    if df is None or df.empty:
        return pd.DataFrame()

    out = pd.DataFrame()
    out["trade_date"] = pd.to_datetime(df["date"]).dt.strftime("%Y-%m-%d")
    out["open"] = pd.to_numeric(df["open"], errors="coerce")
    out["high"] = pd.to_numeric(df["high"], errors="coerce")
    out["low"] = pd.to_numeric(df["low"], errors="coerce")
    out["close"] = pd.to_numeric(df["close"], errors="coerce")

    if "volume" in df.columns:
        out["volume"] = pd.to_numeric(df["volume"], errors="coerce") / 100.0
    else:
        out["volume"] = None

    if "amount" in df.columns:
        out["amount"] = pd.to_numeric(df["amount"], errors="coerce")
    else:
        out["amount"] = None

    # daily 没有直接给涨跌幅，自己算一个
    out["pct_chg"] = out["close"].pct_change() * 100.0

    if "turnover" in df.columns:
        out["turnover"] = pd.to_numeric(df["turnover"], errors="coerce") * 100.0
    else:
        out["turnover"] = None

    out["source"] = "daily_sina"
    out["code"] = normalize_code(code)

    return out.dropna(subset=["trade_date", "open", "high", "low", "close"])


def _standardize_from_tx(code: str, df: pd.DataFrame) -> pd.DataFrame:
    """
    stock_zh_a_hist_tx 返回英文列：
    date open close high low amount

    注意：
    - 这里的 amount 实际文档写的是手，类似成交量
    - 没有成交额、换手率
    """
    if df is None or df.empty:
        return pd.DataFrame()

    out = pd.DataFrame()
    out["trade_date"] = pd.to_datetime(df["date"]).dt.strftime("%Y-%m-%d")
    out["open"] = pd.to_numeric(df["open"], errors="coerce")
    out["high"] = pd.to_numeric(df["high"], errors="coerce")
    out["low"] = pd.to_numeric(df["low"], errors="coerce")
    out["close"] = pd.to_numeric(df["close"], errors="coerce")

    # 腾讯这个 amount 文档写的是“手”，这里放到 volume
    out["volume"] = pd.to_numeric(df["amount"], errors="coerce") if "amount" in df.columns else None

    # 腾讯接口没有成交额
    out["amount"] = None

    # 自己计算涨跌幅
    out["pct_chg"] = out["close"].pct_change() * 100.0

    # 腾讯接口没有换手率
    out["turnover"] = None

    out["source"] = "hist_tx"
    out["code"] = normalize_code(code)

    return out.dropna(subset=["trade_date", "open", "high", "low", "close"])


def fetch_daily_bars(code: str, start_date: str, end_date: str) -> pd.DataFrame:
    """
    三层兜底：
    1. stock_zh_a_hist_tx    腾讯，优先使用
    2. stock_zh_a_hist       东财，备用，字段最完整
    3. stock_zh_a_daily      新浪，最后备用，频繁访问可能封 IP

    返回统一字段：
    code, trade_date, open, high, low, close, volume, amount, pct_chg, turnover, source
    """
    plain_code = normalize_code(code)
    market_code = code_with_market(code)

    errors = []

    providers = [
        ("hist_tx", lambda: _standardize_from_tx(
            plain_code,
            ak.stock_zh_a_hist_tx(
                symbol=market_code,
                start_date=start_date,
                end_date=end_date,
                adjust="",
                timeout=15,
            )
        )),
        ("hist_em", lambda: _standardize_from_hist(
            plain_code,
            ak.stock_zh_a_hist(
                symbol=plain_code,
                period="daily",
                start_date=start_date,
                end_date=end_date,
                adjust="",
                timeout=15,
            )
        )),
        ("daily_sina", lambda: _standardize_from_daily(
            plain_code,
            ak.stock_zh_a_daily(
                symbol=market_code,
                start_date=start_date,
                end_date=end_date,
                adjust="",
            )
        )),
    ]

    for provider_name, func in providers:
        for attempt in range(1, 4):
            try:
                df = func()

                if df is not None and not df.empty:
                    df = df.sort_values("trade_date").reset_index(drop=True)
                    print(f"[fetch_daily_bars] code={plain_code}, provider={provider_name}, rows={len(df)}")
                    return df

                errors.append(f"{provider_name} attempt={attempt}: empty result")

            except Exception as e:
                errors.append(f"{provider_name} attempt={attempt}: {repr(e)}")

            _sleep_before_retry()

    raise RuntimeError(
        f"fetch_daily_bars failed: code={plain_code}, "
        f"start_date={start_date}, end_date={end_date}, errors={errors}"
    )


def fetch_index_daily_bars(symbol: str) -> pd.DataFrame:
    df = ak.stock_zh_index_daily(symbol=symbol)
    if df is None:
        raise RuntimeError(f"fetch_index_daily_bars failed: symbol={symbol}, result is None")
    return df