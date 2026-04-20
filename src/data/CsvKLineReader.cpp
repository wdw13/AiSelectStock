#include "data/CsvKLineReader.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

QVector<KLineBar> CsvKLineReader::readFromFile(const QString& filePath)
{
    QVector<KLineBar> bars;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return bars;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    bool firstLine = true;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (firstLine) {
            firstLine = false;
            continue; // 跳过表头
        }

        QStringList parts = line.split(",");
        if (parts.size() < 7) {
            continue;
        }

        KLineBar bar;
        bar.date   = QDate::fromString(parts[0].trimmed(), "yyyy-MM-dd");
        bar.open   = parts[1].toDouble();
        bar.high   = parts[2].toDouble();
        bar.low    = parts[3].toDouble();
        bar.close  = parts[4].toDouble();
        bar.volume = parts[5].toDouble();
        bar.amount = parts[6].toDouble();

        if (!bar.date.isValid()) {
            continue;
        }

        bars.push_back(bar);
    }

    return bars;
}