#include "core/KLineAggregator.h"

QVector<KLineBar> KLineAggregator::toWeekly(const QVector<KLineBar>& dailyBars)
{
    QVector<KLineBar> result;
    if (dailyBars.isEmpty()) {
        return result;
    }

    KLineBar current = dailyBars.first();
    int currentWeek = current.date.weekNumber();
    int currentYear = current.date.year();

    for (int i = 1; i < dailyBars.size(); ++i) {
        const KLineBar& bar = dailyBars[i];

        int week = bar.date.weekNumber();
        int year = bar.date.year();

        if (week == currentWeek && year == currentYear) {
            current.high += 0; // 占位，下面重写
            if (bar.high > current.high) current.high = bar.high;
            if (bar.low < current.low) current.low = bar.low;
            current.close = bar.close;
            current.date = bar.date;
            current.volume += bar.volume;
            current.amount += bar.amount;
        } else {
            result.push_back(current);
            current = bar;
            currentWeek = week;
            currentYear = year;
        }
    }

    result.push_back(current);
    return result;
}

QVector<KLineBar> KLineAggregator::toMonthly(const QVector<KLineBar>& dailyBars)
{
    QVector<KLineBar> result;
    if (dailyBars.isEmpty()) {
        return result;
    }

    KLineBar current = dailyBars.first();
    int currentYear = current.date.year();
    int currentMonth = current.date.month();

    for (int i = 1; i < dailyBars.size(); ++i) {
        const KLineBar& bar = dailyBars[i];

        int year = bar.date.year();
        int month = bar.date.month();

        if (year == currentYear && month == currentMonth) {
            if (bar.high > current.high) current.high = bar.high;
            if (bar.low < current.low) current.low = bar.low;
            current.close = bar.close;
            current.date = bar.date;
            current.volume += bar.volume;
            current.amount += bar.amount;
        } else {
            result.push_back(current);
            current = bar;
            currentYear = year;
            currentMonth = month;
        }
    }

    result.push_back(current);
    return result;
}