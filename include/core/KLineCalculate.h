#pragma once

#include <QVector>
#include "model/KLineData.h"

class KLineCalculate
{
public:
    static QVector<KLineData> toWeekly(const QVector<KLineData>& dailyBars);
    static QVector<KLineData> toMonthly(const QVector<KLineData>& dailyBars);
};