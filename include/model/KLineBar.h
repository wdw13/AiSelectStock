#pragma once

#include <QDate>

struct KLineBar
{
    QDate date;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
    double amount = 0.0;
};