#pragma once

#include <QString>
#include <QVector>

#include "data/DataBase.h"

struct StockSelectAlgResult
{
    QString code;
    QString name;

    double score = 0.0;

    double price = 0.0;
    double volume = 0.0;
    double amount = 0.0;
    double turnover = 0.0;

    QString strategyName;
    QString reason;
};

class StockSelectorAlg
{
public:
    static bool evaluateAiStock(const StockItem &stock,
                                const QVector<KLineData> &bars,
                                StockSelectAlgResult *out);

    static bool evaluateTraditionalStock(const StockItem &stock,
                                         const QVector<KLineData> &bars,
                                         StockSelectAlgResult *out);

    static QVector<StockSelectAlgResult> sortAndLimit(QVector<StockSelectAlgResult> results,
                                                      int limit = 20);
};