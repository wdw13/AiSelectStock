#pragma once

#include <QString>
#include <QVector>

#include "model/KLineData.h"

struct StockItem
    {
        QString code;
        QString name;
        QString market;
        QString board;
        QString pinyin;
        QString pinyinAbbr;
    };


class DataBase
{
public:
    explicit DataBase(const QString& dbPath);

    QVector<KLineData> loadDailyBars(const QString& code) const;
    QVector<StockItem> loadAllStocks() const;
    QVector<StockItem> searchStocks(const QString &keyword, int limit = 20) const;

private:
    QString m_dbPath;
};