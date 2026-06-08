# AiSelectStock

AI 选股工具。

> 把眼光交给 AI，把选择交给自己。

AiSelectStock 是一个基于 Qt/C++ 开发的桌面端股票分析与选股工具，支持股票数据同步、K 线展示、股票搜索、AI 综合评分选股、传统策略选股和选股进度实时显示。

项目当前阶段以本地行情数据和技术指标计算为核心，不依赖外部大模型接口，不修改数据库结构，所有选股指标均在运行时根据本地 K 线数据临时计算。

---

## 1. 项目简介

本项目主要用于 A 股本地行情分析和选股辅助。

核心流程如下：

```text
同步股票数据
↓
写入本地 SQLite 数据库
↓
主程序读取本地股票与 K 线数据
↓
计算技术指标
↓
执行 AI 综合评分选股 / 传统策略选股
↓
在界面右侧展示选股结果
```

当前已实现：

* 股票列表同步
* 日 K 数据同步
* 上证指数展示
* 股票搜索
* K 线数据展示
* AI 综合评分选股
* 传统策略选股
* 选股实时进度条
* 选股完成后自动刷新结果
* SQLite 本地数据存储

---

## 2. 技术栈

### 2.1 客户端

| 项目     | 说明         |
| ------ | ---------- |
| 开发语言   | C++        |
| GUI 框架 | Qt Widgets |
| 数据库访问  | Qt SQL     |
| 构建工具   | CMake      |
| C++ 标准 | C++17      |
| 数据库    | SQLite     |

### 2.2 数据同步脚本

| 项目   | 说明       |
| ---- | -------- |
| 开发语言 | Python   |
| 行情接口 | AkShare  |
| 数据处理 | pandas   |
| 拼音转换 | pypinyin |
| 数据库  | sqlite3  |

---

## 3. 环境要求

### 3.1 C++ / Qt 环境

建议环境：

```text
Qt 5.15+ 或 Qt 6.x
CMake 3.16+
C++17
MSVC / MinGW / GCC / Clang
```

Windows 推荐：

```text
Qt 6.x + MSVC 2022 64bit
CMake 3.16+
Visual Studio 2022
```

项目 CMake 已启用：

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

并依赖 Qt 模块：

```text
Widgets
Sql
```

---

### 3.2 Python 环境

建议使用 Python 3.10 及以上版本。

原因是同步脚本中使用了 Python 3.10 的类型写法，例如：

```python
Path | str | None
```

安装依赖：

```bash
pip install akshare pandas pypinyin
```

如需隔离环境，建议使用虚拟环境：

```bash
python -m venv .venv
```

Windows PowerShell 激活：

```powershell
.venv\Scripts\activate
```

然后安装依赖：

```powershell
pip install akshare pandas pypinyin
```

---

## 4. 项目目录结构

当前项目主要目录结构如下：

```text
AiSelectStock
├─ CMakeLists.txt
├─ README.md
├─ data
│  └─ database.db
│
├─ include
│  ├─ MainWindow.h
│  │
│  ├─ alg
│  │  └─ StockSelectorAlg.h
│  │
│  ├─ core
│  │  └─ KLineCalculate.h
│  │
│  ├─ data
│  │  ├─ DataBase.h
│  │  └─ DataPath.h
│  │
│  ├─ model
│  │  ├─ KLineData.h
│  │  └─ KLineTime.h
│  │
│  └─ ui
│     ├─ ClickStockWidget.h
│     ├─ DisplayInterfaceWidget.h
│     └─ MatchResultWidget.h
│
├─ src
│  ├─ CmakeLists.txt
│  ├─ main.cpp
│  ├─ MainWindow.cpp
│  │
│  ├─ alg
│  │  └─ StockSelectorAlg.cpp
│  │
│  ├─ core
│  │  └─ KLineCalculate.cpp
│  │
│  ├─ data
│  │  ├─ DataBase.cpp
│  │  └─ DataPath.cpp
│  │
│  └─ ui
│     ├─ ClickStockWidget.cpp
│     ├─ DisplayInterfaceWidget.cpp
│     └─ MatchResultWidget.cpp
│
└─ tools
   ├─ ak_client.py
   ├─ db.py
   ├─ pinyin_util.py
   └─ sync_market_data.py
```

---

## 5. 核心模块说明

### 5.1 主窗口模块

文件：

```text
include/MainWindow.h
src/MainWindow.cpp
```

主要职责：

* 初始化主界面
* 管理股票搜索
* 管理 K 线显示
* 管理右侧选股结果列表
* 处理“同步股票数据”按钮
* 处理“AI选股 / 传统选股”入口
* 显示选股进度条
* 调用算法模块获取选股结果

核心接口：

```text
requestAiSelectResults()
requestTraditionalSelectResults()
runStockSelect()
```

其中：

```text
runStockSelect()
```

负责创建实时进度条，并调用对应的选股接口。

---

### 5.2 算法模块

文件：

```text
include/alg/StockSelectorAlg.h
src/alg/StockSelectorAlg.cpp
```

主要职责：

* 计算股票技术指标
* 执行 AI 综合评分
* 执行传统策略判断
* 输出统一的选股结果结构

当前提供两个核心接口：

```text
evaluateAiStock()
evaluateTraditionalStock()
```

以及结果排序接口：

```text
sortAndLimit()
```

算法模块不直接操作 UI，也不直接操作数据库，只接收：

```text
StockItem
QVector<KLineData>
```

然后返回算法结果。

这样可以保持：

```text
界面层负责展示
数据库层负责读取
算法层负责计算
```

---

### 5.3 数据库模块

文件：

```text
include/data/DataBase.h
src/data/DataBase.cpp
```

主要职责：

* 打开本地 SQLite 数据库
* 查询股票列表
* 查询股票日 K 数据
* 搜索股票
* 为界面和算法模块提供数据

当前主要数据表：

```text
stocks
daily_bars
sync_state
```

---

### 5.4 数据路径模块

文件：

```text
include/data/DataPath.h
src/data/DataPath.cpp
```

主要职责：

* 管理项目运行时数据路径
* 获取数据库路径
* 获取 Python 同步脚本路径

默认数据库路径：

```text
data/database.db
```

---

### 5.5 数据同步脚本

文件：

```text
tools/sync_market_data.py
tools/ak_client.py
tools/db.py
tools/pinyin_util.py
```

主要职责：

* 通过 AkShare 获取股票列表
* 过滤普通 A 股
* 同步上证指数
* 同步股票日 K 数据
* 写入 SQLite 数据库
* 输出同步进度给 Qt 主程序

同步脚本输出格式包括：

```text
PHASE|阶段提示
PROGRESS|当前数量|总数量|股票代码|状态|消息
```

Qt 主程序可以根据这些输出更新同步进度界面。

---

## 6. 数据库说明

项目默认使用 SQLite 数据库：

```text
data/database.db
```

### 6.1 stocks 表

用于保存股票基础信息。

主要字段：

| 字段          | 说明            |
| ----------- | ------------- |
| code        | 股票代码          |
| name        | 股票名称          |
| market      | 市场，例如 SH / SZ |
| board       | 板块            |
| is_normal_a | 是否普通 A 股      |
| pinyin      | 股票名称全拼        |
| pinyin_abbr | 股票名称拼音首字母     |
| status      | 股票状态          |

---

### 6.2 daily_bars 表

用于保存日 K 数据。

主要字段：

| 字段         | 说明   |
| ---------- | ---- |
| code       | 股票代码 |
| trade_date | 交易日期 |
| open       | 开盘价  |
| high       | 最高价  |
| low        | 最低价  |
| close      | 收盘价  |
| volume     | 成交量  |
| amount     | 成交额  |
| pct_chg    | 涨跌幅  |
| turnover   | 换手率  |
| adj_type   | 复权类型 |

---

### 6.3 sync_state 表

用于保存每只股票的数据同步状态。

主要字段：

| 字段              | 说明      |
| --------------- | ------- |
| code            | 股票代码    |
| last_trade_date | 最近同步交易日 |
| last_sync_time  | 最近同步时间  |
| sync_status     | 同步状态    |
| message         | 同步消息    |

---

## 7. 功能说明

### 7.1 股票数据同步

点击界面中的“同步股票数据”后，程序会调用 Python 脚本：

```text
tools/sync_market_data.py
```

同步内容包括：

```text
股票列表
上证指数
普通 A 股日 K 数据
```

当前股票过滤规则：

```text
保留沪深主板普通 A 股
过滤 ST / *ST 股票
过滤创业板、科创板、北交所等股票
```

当前默认保留：

```text
上海主板：600 / 601 / 603 / 605
深圳主板：000 / 001
```

---

### 7.2 股票搜索

支持通过以下方式搜索股票：

```text
股票代码
股票名称
拼音全拼
拼音首字母
```

例如：

```text
000001
平安银行
pinganyinhang
payh
```

---

### 7.3 K 线展示

主界面支持展示股票 K 线数据。

当前数据来源于本地数据库：

```text
daily_bars
```

默认指数：

```text
sh000001 上证指数
```

---

### 7.4 AI 选股

AI 选股当前阶段不是调用外部大模型，而是基于本地 K 线数据进行多因子综合评分。

算法入口：

```text
StockSelectorAlg::evaluateAiStock()
```

评分范围：

```text
0 - 100 分
```

当前默认筛选阈值：

```text
65 分
```

AI 综合评分主要包含四类因子：

```text
趋势分
买点分
量能分
风险分
```

#### 趋势分

主要判断：

```text
当前价是否站上 MA20
当前价是否站上 MA60
MA5 是否大于 MA10
MA10 是否大于 MA20
MA20 是否大于 MA60
```

用于判断股票是否处于相对健康的上升趋势中。

#### 买点分

主要判断：

```text
近 5 日涨幅
近 20 日涨幅
近 60 日涨幅
当前价在近 60 日高低点中的位置
是否短期过热
是否处于相对合理位置
```

用于避免追高，同时保留趋势开始走强的股票。

#### 量能分

主要判断：

```text
近 5 日均量 / 近 20 日均量
成交额
换手率
是否温和放量
是否量能异常
```

用于判断是否有资金参与。

#### 风险分

主要判断：

```text
近 60 日最大回撤
近 30 日波动率
近 5 日是否涨幅过大
当前位置是否过高
换手率是否异常
```

用于降低高波动、高回撤、短期过热股票的排名。

---

### 7.5 传统选股

传统选股基于固定技术策略规则。

算法入口：

```text
StockSelectorAlg::evaluateTraditionalStock()
```

当前内置策略：

```text
短线活跃股
阳包阴反包
均线多头趋势
```

#### 短线活跃股

主要判断：

```text
今日涨幅处于合理区间
换手率较活跃
成交量放大
收盘价站上 MA5
MA5 大于 MA10
```

适合筛选短线资金关注的股票。

#### 阳包阴反包

主要判断：

```text
昨日下跌
今日明显上涨
今日收盘价反包昨日实体
今日成交量放大
```

适合筛选短线情绪修复和资金重新介入的股票。

#### 均线多头趋势

主要判断：

```text
当前价在 MA20 上方
MA5 > MA10 > MA20
MA20 > MA60
近 20 日涨幅为正但不过热
近 60 日最大回撤不过大
量能处于合理范围
```

适合筛选趋势相对稳定的股票。

---

### 7.6 实时选股进度条

点击“开始选股”后，程序会弹出实时进度条。

进度条显示：

```text
当前处理数量 / 股票总数量
当前分析的股票代码
当前分析的股票名称
```

选股完成后：

```text
进度条自动关闭
右侧选股结果自动刷新
```

如果没有符合条件的股票，会弹出提示：

```text
暂无符合条件的股票。请先同步股票数据，或者适当放宽选股条件。
```

---

## 8. 编译运行

### 8.1 使用 Qt Creator

推荐方式：

1. 使用 Qt Creator 打开项目根目录下的 `CMakeLists.txt`
2. 选择 Qt Kit
3. 配置 CMake
4. 构建项目
5. 运行项目

---

### 8.2 使用命令行构建

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build --config Release
```

Windows + Visual Studio 示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## 9. 数据同步方式

### 9.1 在软件界面中同步

启动软件后，点击：

```text
同步股票数据
```

等待同步完成即可。

---

### 9.2 命令行手动同步

在项目根目录执行：

```bash
python tools/sync_market_data.py
```

限制同步数量，用于测试：

```bash
python tools/sync_market_data.py --limit 20
```

指定线程数：

```bash
python tools/sync_market_data.py --workers 6
```

从指定偏移开始同步：

```bash
python tools/sync_market_data.py --limit 100 --offset 200
```

---

## 10. 常见问题

### 10.1 选股结果为空

可能原因：

```text
没有同步股票数据
daily_bars 表为空
K 线数量不足 80 条
AI 评分阈值过高
传统策略条件较严格
行情接口返回数据不完整
```

解决方式：

```text
先点击“同步股票数据”
检查 data/database.db 是否存在
确认 daily_bars 表中有数据
适当降低 AI 评分阈值
适当放宽传统策略条件
```

---

### 10.2 同步股票数据失败

可能原因：

```text
网络异常
AkShare 接口不可用
访问频率过高
Python 依赖未安装
Python 版本过低
```

解决方式：

```bash
pip install akshare pandas pypinyin
```

确认 Python 版本：

```bash
python --version
```

建议使用：

```text
Python 3.10+
```

---

### 10.3 编译时报 Qt SQL 找不到

确认 CMake 中已经查找并链接 Qt SQL：

```cmake
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
)
```

---

## 11. 后续优化方向

后续可以继续扩展：

```text
将选股计算放入 QThread，避免主线程阻塞
增加选股结果的入选原因展示
增加策略名称展示
增加风险提示展示
增加 AI 文字分析
接入 DeepSeek / 通义千问 / OpenAI 等大模型
增加新闻舆情分析
增加历史相似形态统计
增加回测功能
增加创业板、科创板、北交所支持
增加用户自定义选股条件
增加选股参数配置界面
```

---

## 12. 免责声明

本项目仅用于学习、研究和辅助分析。

项目中的 AI 选股、传统选股、技术指标评分、策略筛选结果均不构成任何投资建议。

股票市场存在风险，任何买卖决策都应由使用者自行判断并承担相应风险。
