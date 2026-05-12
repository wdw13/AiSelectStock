#include "data/DataBase.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDate>
#include <QDebug>
#include <QUuid>

DataBase::DataBase(const QString& dbPath)
    : m_dbPath(dbPath)
{
}

QVector<KLineData> DataBase::loadDailyBars(const QString& code) const
{
    QVector<KLineData> bars;

    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(m_dbPath);

        if (!db.open()) {
            qWarning() << "[DataBase] open db failed in loadDailyBars:" << db.lastError().text();
            return bars;
        }

        QSqlQuery query(db);
        query.prepare(R"(
            SELECT trade_date, open, high, low, close, volume, amount, turnover
            FROM daily_bars
            WHERE code = :code AND adj_type = 'none'
            ORDER BY trade_date ASC
        )");
        query.bindValue(":code", code);

        if (!query.exec()) {
            qWarning() << "[DataBase] query failed in loadDailyBars:" << query.lastError().text();
            db.close();
            return bars;
        }

        while (query.next()) {
            KLineData bar;
            bar.date = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");
            bar.open = query.value(1).toDouble();
            bar.high = query.value(2).toDouble();
            bar.low = query.value(3).toDouble();
            bar.close = query.value(4).toDouble();
            bar.volume = query.value(5).toDouble();
            bar.amount = query.value(6).toDouble();
            bar.turnover = query.value(7).toDouble();
            if (bar.date.isValid()) {
                bars.push_back(bar);
            }
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return bars;
}

QVector<StockItem> DataBase::loadAllStocks() const
{
    QVector<StockItem> result;

    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(m_dbPath);

        if (!db.open()) {
            qWarning() << "[DataBase] open db failed in loadAllStocks:" << db.lastError().text();
            return result;
        }

        QSqlQuery query(db);
        query.prepare(R"(
            SELECT code, name, market, board, pinyin, pinyin_abbr
            FROM stocks
            WHERE is_normal_a = 1
            ORDER BY code ASC
        )");

        if (!query.exec()) {
            qWarning() << "[DataBase] query failed in loadAllStocks:" << query.lastError().text();
            db.close();
            return result;
        }

        while (query.next()) {
            StockItem item;
            item.code = query.value(0).toString();
            item.name = query.value(1).toString();
            item.market = query.value(2).toString();
            item.board = query.value(3).toString();
            item.pinyin = query.value(4).toString();
            item.pinyinAbbr = query.value(5).toString();
            result.push_back(item);
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return result;
}

QVector<StockItem> DataBase::searchStocks(const QString& keyword, int limit) const
{
    QVector<StockItem> result;

    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        return result;
    }

    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(m_dbPath);

        if (!db.open()) {
            qWarning() << "[DataBase] open db failed in searchStocks:" << db.lastError().text();
            return result;
        }

        QString sql = R"(
            SELECT code, name, market, board, pinyin, pinyin_abbr
            FROM stocks
            WHERE is_normal_a = 1
              AND (
                  code LIKE :prefix
                  OR name LIKE :contains
                  OR pinyin LIKE :prefix
                  OR pinyin_abbr LIKE :prefix
              )
            ORDER BY
              CASE
                  WHEN code = :exact THEN 0
                  WHEN name = :exact THEN 1
                  WHEN code LIKE :prefix THEN 2
                  WHEN pinyin_abbr LIKE :prefix THEN 3
                  WHEN pinyin LIKE :prefix THEN 4
                  WHEN name LIKE :contains THEN 5
                  ELSE 6
              END,
              code ASC
            LIMIT %1
        )";

        sql = sql.arg(limit > 0 ? limit : 20);

        QSqlQuery query(db);
        query.prepare(sql);

        query.bindValue(":exact", trimmed);
        query.bindValue(":prefix", trimmed + "%");
        query.bindValue(":contains", "%" + trimmed + "%");

        if (!query.exec()) {
            qWarning() << "[DataBase] query failed in searchStocks:" << query.lastError().text();
            db.close();
            return result;
        }

        while (query.next()) {
            StockItem item;
            item.code = query.value(0).toString();
            item.name = query.value(1).toString();
            item.market = query.value(2).toString();
            item.board = query.value(3).toString();
            item.pinyin = query.value(4).toString();
            item.pinyinAbbr = query.value(5).toString();
            result.push_back(item);
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return result;
}
