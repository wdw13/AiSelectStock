#pragma once

#include <QFrame>
#include <QString>

class ResultCard : public QFrame
{
public:
    explicit ResultCard(const QString &name,
                        const QString &code,
                        const QString &score,
                        QWidget *parent = nullptr);
};