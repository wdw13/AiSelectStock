#pragma once

#include <QVector>
#include <QString>
#include "model/KLineBar.h"

class CsvKLineReader
{
public:
    static QVector<KLineBar> readFromFile(const QString& filePath);
};