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