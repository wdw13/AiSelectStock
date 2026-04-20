import os
from pathlib import Path

import akshare as ak
import pandas as pd


def main():
    # 以脚本所在目录的上一级作为项目根目录
    project_root = Path(__file__).resolve().parent.parent
    data_dir = project_root / "data"
    data_dir.mkdir(parents=True, exist_ok=True)

    out_file = data_dir / "sh000001_daily.csv"

    # 获取上证指数历史日线
    df = ak.stock_zh_index_daily_em(
        symbol="sh000001",
        start_date="20180101",
        end_date="20300101",
    )

    # 只保留你后面画 K 线最需要的列
    df = df[["date", "open", "high", "low", "close", "volume", "amount"]].copy()

    # 类型清洗
    df["date"] = pd.to_datetime(df["date"]).dt.strftime("%Y-%m-%d")
    for col in ["open", "high", "low", "close", "volume", "amount"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    df = df.dropna().reset_index(drop=True)

    # 保存成 UTF-8 BOM，Qt 里更省心
    df.to_csv(out_file, index=False, encoding="utf-8-sig")

    print(f"已生成: {out_file}")
    print(df.head())
    print(df.tail())
    print(f"总条数: {len(df)}")


if __name__ == "__main__":
    main()