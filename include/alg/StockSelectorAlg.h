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

struct CustomSelectConfig
{
    bool enableRecentLimitUp = true;
    int limitUpLookbackDays = 20;
    int limitUpMinCount = 1;

    bool enableMaLongOrder = true;
    bool enableMaUp = true;
    bool enablePriceAboveMa5 = true;

    bool enableAlongMa5 = false;
    int alongMa5Days = 7;
    int alongMa5MinDays = 5;

    bool enableShortBottom = false;
    int bottomLookbackDays = 30;
    double bottomMinRecoverPct = 5.0;
    double bottomMaxRecoverPct = 35.0;

    bool enableWeeklyMacdUp = false;

    bool enableRet20Max = false;
    double ret20MaxPct = 35.0;

    bool enableTurnoverRange = false;
    double turnoverMin = 1.0;
    double turnoverMax = 12.0;

    bool enableAmountMin = false;
    double amountMin = 30000000.0;

    bool enableDrawdownMax = false;
    double drawdownMaxPct = 30.0;

    bool enableVolumeRatioRange = false;
    double volumeRatioMin = 0.8;
    double volumeRatioMax = 2.8;
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

    static bool evaluateCustomStock(const StockItem &stock,
                                const QVector<KLineData> &bars,
                                const CustomSelectConfig &config,
                                StockSelectAlgResult *out);

    static QVector<StockSelectAlgResult> sortAndLimit(QVector<StockSelectAlgResult> results,
                                                      int limit = 20);
};