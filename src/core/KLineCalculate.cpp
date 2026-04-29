#include "core/KLineCalculate.h"

QVector<KLineData> KLineCalculate::toWeekly(const QVector<KLineData>& dailyBars)
{
    QVector<KLineData> result;
    if (dailyBars.isEmpty()) {
        return result;
    }

    KLineData current = dailyBars.first();

    int currentYear = 0;
    int currentWeek = current.date.weekNumber(&currentYear);

    for (int i = 1; i < dailyBars.size(); ++i) {
        const KLineData& bar = dailyBars[i];

        int year = 0;
        int week = bar.date.weekNumber(&year);

        if (week == currentWeek && year == currentYear) {
            if (bar.high > current.high) {
                current.high = bar.high;
            }

            if (bar.low < current.low) {
                current.low = bar.low;
            }

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

QVector<KLineData> KLineCalculate::toMonthly(const QVector<KLineData>& dailyBars)
{
    QVector<KLineData> result;
    if (dailyBars.isEmpty()) {
        return result;
    }

    KLineData current = dailyBars.first();
    int currentYear = current.date.year();
    int currentMonth = current.date.month();

    for (int i = 1; i < dailyBars.size(); ++i) {
        const KLineData& bar = dailyBars[i];

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