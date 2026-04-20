#pragma once

#include <QVector>
#include "model/KLineBar.h"

class KLineAggregator
{
public:
    static QVector<KLineBar> toWeekly(const QVector<KLineBar>& dailyBars);
    static QVector<KLineBar> toMonthly(const QVector<KLineBar>& dailyBars);
};