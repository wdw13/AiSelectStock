#include "alg/StockSelectorAlg.h"

#include <algorithm>
#include <cmath>
#include <QStringList>

namespace
{
    static double averageClose(const QVector<KLineData> &bars, int period, int endOffset = 0)
    {
        const int end = bars.size() - 1 - endOffset;

        if (period <= 0 || end < 0 || end - period + 1 < 0) {
            return 0.0;
        }

        double sum = 0.0;

        for (int i = end - period + 1; i <= end; ++i) {
            sum += bars[i].close;
        }

        return sum / period;
    }

    static double averageVolume(const QVector<KLineData> &bars, int period, int endOffset = 0)
    {
        const int end = bars.size() - 1 - endOffset;

        if (period <= 0 || end < 0 || end - period + 1 < 0) {
            return 0.0;
        }

        double sum = 0.0;

        for (int i = end - period + 1; i <= end; ++i) {
            sum += bars[i].volume;
        }

        return sum / period;
    }

    static double pctChangeFromDaysAgo(const QVector<KLineData> &bars, int days)
    {
        const int last = bars.size() - 1;
        const int prev = last - days;

        if (prev < 0 || bars[prev].close <= 0.0) {
            return 0.0;
        }

        return (bars[last].close - bars[prev].close) / bars[prev].close * 100.0;
    }

    static double oneDayPctChange(const QVector<KLineData> &bars, int endOffset = 0)
    {
        const int cur = bars.size() - 1 - endOffset;
        const int prev = cur - 1;

        if (cur <= 0 || prev < 0 || bars[prev].close <= 0.0) {
            return 0.0;
        }

        return (bars[cur].close - bars[prev].close) / bars[prev].close * 100.0;
    }

    static double maxHighInLast(const QVector<KLineData> &bars, int period)
    {
        if (bars.isEmpty()) {
            return 0.0;
        }

        const int start = std::max(0, static_cast<int>(bars.size() - period));
        double value = bars[start].high;

        for (int i = start; i < bars.size(); ++i) {
            value = std::max(value, bars[i].high);
        }

        return value;
    }

    static double minLowInLast(const QVector<KLineData> &bars, int period)
    {
        if (bars.isEmpty()) {
            return 0.0;
        }

        const int start = std::max(0, static_cast<int>(bars.size() - period));
        double value = bars[start].low;

        for (int i = start; i < bars.size(); ++i) {
            value = std::min(value, bars[i].low);
        }

        return value;
    }

    static double maxDrawdownInLast(const QVector<KLineData> &bars, int period)
    {
        if (bars.isEmpty()) {
            return 0.0;
        }

        const int start = std::max(0, static_cast<int>(bars.size() - period));

        double peak = bars[start].close;
        double maxDrawdown = 0.0;

        for (int i = start; i < bars.size(); ++i) {
            peak = std::max(peak, bars[i].close);

            if (peak > 0.0) {
                const double drawdown = (peak - bars[i].close) / peak * 100.0;
                maxDrawdown = std::max(maxDrawdown, drawdown);
            }
        }

        return maxDrawdown;
    }

    static double volatilityInLast(const QVector<KLineData> &bars, int period)
    {
        if (bars.size() < period + 1) {
            return 0.0;
        }

        const int start = std::max(1, static_cast<int>(bars.size() - period));

        QVector<double> returns;
        returns.reserve(period);

        for (int i = start; i < bars.size(); ++i) {
            if (bars[i - 1].close <= 0.0) {
                continue;
            }

            const double r = (bars[i].close - bars[i - 1].close) / bars[i - 1].close * 100.0;
            returns.push_back(r);
        }

        if (returns.isEmpty()) {
            return 0.0;
        }

        double avg = 0.0;

        for (double r : returns) {
            avg += r;
        }

        avg /= returns.size();

        double variance = 0.0;

        for (double r : returns) {
            const double diff = r - avg;
            variance += diff * diff;
        }

        variance /= returns.size();

        return std::sqrt(variance);
    }

    static void fillLastQuote(const StockItem &stock,
                              const QVector<KLineData> &bars,
                              StockSelectAlgResult *out)
    {
        if (!out || bars.isEmpty()) {
            return;
        }

        const KLineData &last = bars.last();

        out->code = stock.code;
        out->name = stock.name;
        out->price = last.close;
        out->volume = last.volume;
        out->amount = last.amount;
        out->turnover = last.turnover;
    }

    static double averageCloseAt(const QVector<KLineData> &bars, int period, int endIndex)
{
    if (period <= 0 || endIndex < 0 || endIndex - period + 1 < 0) {
        return 0.0;
    }

    double sum = 0.0;

    for (int i = endIndex - period + 1; i <= endIndex; ++i) {
        sum += bars[i].close;
    }

    return sum / period;
}

static bool hasRecentLimitUp(const QVector<KLineData> &bars, int days)
{
    const int n = static_cast<int>(bars.size());
    const int start = std::max(1, n - days);

    for (int i = start; i < n; ++i) {
        if (bars[i - 1].close <= 0.0) {
            continue;
        }

        const double pct = (bars[i].close - bars[i - 1].close) / bars[i - 1].close * 100.0;

        if (pct >= 9.5) {
            return true;
        }
    }

    return false;
}

static bool isShortBottomRecovered(const QVector<KLineData> &bars)
{
    const int n = static_cast<int>(bars.size());

    if (n < 80) {
        return false;
    }

    const int lookback = 30;
    const int start = std::max(0, n - lookback);

    double bottomLow = bars[start].low;
    int bottomIndex = start;

    for (int i = start; i < n; ++i) {
        if (bars[i].low < bottomLow) {
            bottomLow = bars[i].low;
            bottomIndex = i;
        }
    }

    const int bottomOffset = n - 1 - bottomIndex;
    const double lastClose = bars.last().close;

    if (bottomLow <= 0.0 || lastClose <= 0.0) {
        return false;
    }

    const double recoverPct = (lastClose - bottomLow) / bottomLow * 100.0;

    // 底部不能是今天刚出现，也不能太久远
    if (bottomOffset < 3 || bottomOffset > 20) {
        return false;
    }

    // 已经从短期底部反弹，但不能涨太高
    if (recoverPct < 5.0 || recoverPct > 35.0) {
        return false;
    }

    return true;
}

static bool isPriceRisingAlongMa5(const QVector<KLineData> &bars)
{
    const int n = static_cast<int>(bars.size());

    if (n < 30) {
        return false;
    }

    int aboveMa5Count = 0;

    for (int i = n - 7; i < n; ++i) {
        const double ma5 = averageCloseAt(bars, 5, i);

        if (ma5 <= 0.0) {
            continue;
        }

        const double close = bars[i].close;
        const double distance = std::abs(close - ma5) / ma5 * 100.0;

        // 收盘在 5 日线上方，或者非常贴近 5 日线，也算沿 5 日线
        if (close >= ma5 || distance <= 2.0) {
            ++aboveMa5Count;
        }
    }

    const double currentMa5 = averageClose(bars, 5);
    const double prevMa5 = averageClose(bars, 5, 1);

    if (currentMa5 <= 0.0 || prevMa5 <= 0.0) {
        return false;
    }

    // 最近 7 天至少 5 天沿 MA5 运行，并且 MA5 自身向上
    return aboveMa5Count >= 5 && currentMa5 > prevMa5;
}

static bool isMa5Ma10Ma20UpWithoutCross(const QVector<KLineData> &bars)
{
    const int n = static_cast<int>(bars.size());

    if (n < 40) {
        return false;
    }

    const double ma5 = averageClose(bars, 5);
    const double ma10 = averageClose(bars, 10);
    const double ma20 = averageClose(bars, 20);

    const double prevMa5 = averageClose(bars, 5, 1);
    const double prevMa10 = averageClose(bars, 10, 1);
    const double prevMa20 = averageClose(bars, 20, 1);

    if (ma5 <= 0.0 || ma10 <= 0.0 || ma20 <= 0.0) {
        return false;
    }

    // 当前三条均线必须多头排列
    if (!(ma5 > ma10 && ma10 > ma20)) {
        return false;
    }

    // 三条均线必须同时向上
    if (!(ma5 > prevMa5 && ma10 > prevMa10 && ma20 > prevMa20)) {
        return false;
    }

    // 最近 8 个交易日不能发生均线交叉，保持 MA5 > MA10 > MA20
    for (int i = n - 8; i < n; ++i) {
        const double m5 = averageCloseAt(bars, 5, i);
        const double m10 = averageCloseAt(bars, 10, i);
        const double m20 = averageCloseAt(bars, 20, i);

        if (!(m5 > m10 && m10 > m20)) {
            return false;
        }
    }

    return true;
}

static QVector<double> buildWeeklyCloseSeries(const QVector<KLineData> &bars)
{
    QVector<double> weeklyCloses;

    if (bars.isEmpty()) {
        return weeklyCloses;
    }

    int currentYear = 0;
    int currentWeek = -1;
    double lastCloseInWeek = 0.0;

    for (const KLineData &bar : bars) {
        if (!bar.date.isValid()) {
            continue;
        }

        int year = 0;
        const int week = bar.date.weekNumber(&year);

        if (currentWeek == -1) {
            currentYear = year;
            currentWeek = week;
            lastCloseInWeek = bar.close;
            continue;
        }

        if (year == currentYear && week == currentWeek) {
            lastCloseInWeek = bar.close;
        } else {
            weeklyCloses.push_back(lastCloseInWeek);

            currentYear = year;
            currentWeek = week;
            lastCloseInWeek = bar.close;
        }
    }

    if (lastCloseInWeek > 0.0) {
        weeklyCloses.push_back(lastCloseInWeek);
    }

    return weeklyCloses;
}

static QVector<double> emaSeries(const QVector<double> &values, int period)
{
    QVector<double> result;

    if (values.isEmpty() || period <= 0) {
        return result;
    }

    result.reserve(values.size());

    const double alpha = 2.0 / (period + 1.0);
    double ema = values.first();

    result.push_back(ema);

    for (int i = 1; i < values.size(); ++i) {
        ema = values[i] * alpha + ema * (1.0 - alpha);
        result.push_back(ema);
    }

    return result;
}

static bool isWeeklyMacdUpTrend(const QVector<KLineData> &bars)
{
    const QVector<double> weeklyCloses = buildWeeklyCloseSeries(bars);

    if (weeklyCloses.size() < 35) {
        return false;
    }

    const QVector<double> ema12 = emaSeries(weeklyCloses, 12);
    const QVector<double> ema26 = emaSeries(weeklyCloses, 26);

    if (ema12.size() != ema26.size()) {
        return false;
    }

    QVector<double> dif;
    dif.reserve(weeklyCloses.size());

    for (int i = 0; i < weeklyCloses.size(); ++i) {
        dif.push_back(ema12[i] - ema26[i]);
    }

    const QVector<double> dea = emaSeries(dif, 9);

    if (dea.size() < 4 || dif.size() < 4) {
        return false;
    }

    const int n = static_cast<int>(dif.size());

    const double hist0 = dif[n - 1] - dea[n - 1];
    const double hist1 = dif[n - 2] - dea[n - 2];
    const double hist2 = dif[n - 3] - dea[n - 3];

    const bool difUp = dif[n - 1] > dif[n - 2] && dif[n - 2] >= dif[n - 3];
    const bool histImprove = hist0 > hist1 && hist1 >= hist2;

    // 周线 MACD 上涨趋势：DIF 连续改善，且柱体同步改善
    return difUp && histImprove;
}
}

bool StockSelectorAlg::evaluateAiStock(const StockItem &stock,
                                       const QVector<KLineData> &bars,
                                       StockSelectAlgResult *out)
{
    if (!out) {
        return false;
    }

    if (bars.size() < 80) {
        return false;
    }

    const KLineData &last = bars.last();

    if (last.close <= 0.0 || last.volume <= 0.0) {
        return false;
    }

    const double ma5 = averageClose(bars, 5);
    const double ma10 = averageClose(bars, 10);
    const double ma20 = averageClose(bars, 20);
    const double ma60 = averageClose(bars, 60);

    const double volumeMa5 = averageVolume(bars, 5);
    const double volumeMa20 = averageVolume(bars, 20);
    const double volumeRatio = volumeMa20 > 0.0 ? volumeMa5 / volumeMa20 : 0.0;

    const double ret5 = pctChangeFromDaysAgo(bars, 5);
    const double ret20 = pctChangeFromDaysAgo(bars, 20);
    const double ret60 = pctChangeFromDaysAgo(bars, 60);

    const double high60 = maxHighInLast(bars, 60);
    const double low60 = minLowInLast(bars, 60);

    const double position60 = high60 > low60
        ? (last.close - low60) / (high60 - low60)
        : 0.0;

    const double drawdown60 = maxDrawdownInLast(bars, 60);
    const double volatility30 = volatilityInLast(bars, 30);

    double trendScore = 0.0;

    if (last.close > ma20) {
        trendScore += 8.0;
    }

    if (last.close > ma60) {
        trendScore += 8.0;
    }

    if (ma5 > ma10) {
        trendScore += 8.0;
    }

    if (ma10 > ma20) {
        trendScore += 8.0;
    }

    if (ma20 > ma60) {
        trendScore += 8.0;
    }

    double timingScore = 0.0;

    if (ret20 > 0.0) {
        timingScore += std::min(8.0, ret20 / 2.0);
    }

    if (ret60 > 0.0) {
        timingScore += std::min(6.0, ret60 / 4.0);
    }

    if (ret5 >= 0.0 && ret5 <= 10.0) {
        timingScore += 6.0;
    } else if (ret5 > 10.0 && ret5 <= 18.0) {
        timingScore += 2.0;
    }

    if (position60 >= 0.35 && position60 <= 0.85) {
        timingScore += 5.0;
    } else if (position60 >= 0.20 && position60 < 0.35) {
        timingScore += 3.0;
    }

    double volumeScore = 0.0;

    if (volumeRatio >= 1.05 && volumeRatio <= 2.50) {
        volumeScore += 12.0;
    } else if (volumeRatio >= 0.80 && volumeRatio < 1.05) {
        volumeScore += 6.0;
    } else if (volumeRatio > 2.50 && volumeRatio <= 4.00) {
        volumeScore += 5.0;
    }

    if (last.amount >= 30000000.0) {
        volumeScore += 4.0;
    }

    if (last.turnover >= 1.0 && last.turnover <= 12.0) {
        volumeScore += 4.0;
    }

    double riskScore = 15.0;

    if (drawdown60 > 35.0) {
        riskScore -= 10.0;
    } else if (drawdown60 > 25.0) {
        riskScore -= 6.0;
    } else if (drawdown60 > 18.0) {
        riskScore -= 3.0;
    }

    if (volatility30 > 6.0) {
        riskScore -= 4.0;
    } else if (volatility30 > 4.0) {
        riskScore -= 2.0;
    }

    if (ret5 > 18.0) {
        riskScore -= 4.0;
    }

    if (position60 > 0.95) {
        riskScore -= 3.0;
    }

    if (last.turnover > 15.0) {
        riskScore -= 4.0;
    }

    riskScore = std::clamp(riskScore, 0.0, 15.0);

    const double score = std::clamp(trendScore + timingScore + volumeScore + riskScore,
                                    0.0,
                                    100.0);

    if (score < 65.0) {
        return false;
    }

    fillLastQuote(stock, bars, out);

    out->score = score;
    out->strategyName = QStringLiteral("AI综合评分");
    out->reason = QStringLiteral("趋势分%1，买点分%2，量能分%3，风险分%4")
        .arg(trendScore, 0, 'f', 0)
        .arg(timingScore, 0, 'f', 0)
        .arg(volumeScore, 0, 'f', 0)
        .arg(riskScore, 0, 'f', 0);

    return true;
}

bool StockSelectorAlg::evaluateTraditionalStock(const StockItem &stock,
                                                const QVector<KLineData> &bars,
                                                StockSelectAlgResult *out)
{
    if (!out) {
        return false;
    }

    if (bars.size() < 80) {
        return false;
    }

    const int n = bars.size();

    const KLineData &last = bars[n - 1];
    const KLineData &prev = bars[n - 2];

    if (last.close <= 0.0 || prev.close <= 0.0 || last.volume <= 0.0) {
        return false;
    }

    const double todayPct = oneDayPctChange(bars, 0);
    const double yesterdayPct = oneDayPctChange(bars, 1);

    const double ma5 = averageClose(bars, 5);
    const double ma10 = averageClose(bars, 10);
    const double ma20 = averageClose(bars, 20);
    const double ma60 = averageClose(bars, 60);

    const double volumeMa5Before = averageVolume(bars, 5, 1);
    const double volumeMa5 = averageVolume(bars, 5);
    const double volumeMa20 = averageVolume(bars, 20);
    const double volumeRatio = volumeMa20 > 0.0 ? volumeMa5 / volumeMa20 : 0.0;

    const double ret20 = pctChangeFromDaysAgo(bars, 20);
    const double drawdown60 = maxDrawdownInLast(bars, 60);

    QStringList strategies;

    double bestScore = 0.0;

    const bool shortActive =
        todayPct >= 3.0 &&
        todayPct <= 6.0 &&
        last.turnover >= 3.0 &&
        last.turnover <= 12.0 &&
        volumeMa5Before > 0.0 &&
        last.volume > volumeMa5Before &&
        last.close > ma5 &&
        ma5 > ma10;

    if (shortActive) {
        strategies << QStringLiteral("短线活跃股");
        bestScore = std::max(bestScore, 88.0 + std::min(6.0, todayPct));
    }

    const bool yangBaoYin =
        yesterdayPct < -2.0 &&
        todayPct > 5.0 &&
        last.close > prev.open &&
        last.open < prev.close &&
        volumeMa5Before > 0.0 &&
        last.volume > volumeMa5Before;

    if (yangBaoYin) {
        strategies << QStringLiteral("阳包阴反包");
        bestScore = std::max(bestScore, 84.0 + std::min(10.0, todayPct));
    }

    const bool trendFollow =
        last.close > ma20 &&
        ma5 > ma10 &&
        ma10 > ma20 &&
        ma20 > ma60 &&
        ret20 > 0.0 &&
        ret20 < 25.0 &&
        drawdown60 < 30.0 &&
        volumeRatio >= 0.8 &&
        volumeRatio <= 2.8;

    if (trendFollow) {
        strategies << QStringLiteral("均线多头趋势");
        bestScore = std::max(bestScore, 78.0 + std::min(12.0, ret20 / 2.0));
    }

    if (strategies.isEmpty()) {
        return false;
    }

    const double multiStrategyBonus = std::max(0, static_cast<int>(strategies.size()) - 1) * 3.0;
    const double score = std::clamp(bestScore + multiStrategyBonus, 0.0, 100.0);

    fillLastQuote(stock, bars, out);

    out->score = score;
    out->strategyName = strategies.join(QStringLiteral(" / "));
    out->reason = QStringLiteral("命中策略：%1").arg(out->strategyName);

    return true;
}

bool StockSelectorAlg::evaluateBsStock(const StockItem &stock,
                                       const QVector<KLineData> &bars,
                                       StockSelectAlgResult *out)
{
    if (!out) {
        return false;
    }

    if (bars.size() < 160) {
        return false;
    }

    const KLineData &last = bars.last();

    if (last.close <= 0.0 || last.volume <= 0.0) {
        return false;
    }

    const bool bottomRecovered = isShortBottomRecovered(bars);
    const bool alongMa5 = isPriceRisingAlongMa5(bars);
    const bool maUpNoCross = isMa5Ma10Ma20UpWithoutCross(bars);
    const bool recentLimitUp = hasRecentLimitUp(bars, 20);
    const bool weeklyMacdUp = isWeeklyMacdUpTrend(bars);

    if (!(bottomRecovered &&
          alongMa5 &&
          maUpNoCross &&
          recentLimitUp &&
          weeklyMacdUp)) {
        return false;
    }

    const double ma5 = averageClose(bars, 5);
    const double ma10 = averageClose(bars, 10);
    const double ma20 = averageClose(bars, 20);

    const double ret20 = pctChangeFromDaysAgo(bars, 20);
    const double drawdown60 = maxDrawdownInLast(bars, 60);
    const double volatility30 = volatilityInLast(bars, 30);

    double score = 80.0;

    // 越贴近 5 日线，越符合“沿 5 日线逐步上涨”
    if (ma5 > 0.0) {
        const double distanceToMa5 = std::abs(last.close - ma5) / ma5 * 100.0;

        if (distanceToMa5 <= 3.0) {
            score += 8.0;
        } else if (distanceToMa5 <= 6.0) {
            score += 4.0;
        } else {
            score -= 5.0;
        }
    }

    // 三条均线越顺，分数越高
    if (ma5 > ma10 && ma10 > ma20) {
        score += 5.0;
    }

    // 近 20 日上涨不能过热
    if (ret20 > 0.0 && ret20 <= 25.0) {
        score += 5.0;
    } else if (ret20 > 35.0) {
        score -= 8.0;
    }

    // 回撤和波动率控制
    if (drawdown60 > 30.0) {
        score -= 8.0;
    }

    if (volatility30 > 6.0) {
        score -= 5.0;
    }

    score = std::clamp(score, 0.0, 100.0);

    fillLastQuote(stock, bars, out);

    out->score = score;
    out->strategyName = QStringLiteral("BS专属");
    out->reason = QStringLiteral("短期底部反弹，沿5日线上行，MA5/10/20多头向上，近日涨停，周线MACD上行");

    return true;
}

QVector<StockSelectAlgResult> StockSelectorAlg::sortAndLimit(QVector<StockSelectAlgResult> results,
                                                             int limit)
{
    std::sort(results.begin(), results.end(),
              [](const StockSelectAlgResult &a, const StockSelectAlgResult &b) {
        return a.score > b.score;
    });

    if (limit > 0 && results.size() > limit) {
        results.resize(limit);
    }

    return results;
}