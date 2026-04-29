#pragma once

#include <QFrame>
#include <QString>

class MatchResultWidget : public QFrame
{
public:
    explicit MatchResultWidget(const QString &name,
                        const QString &code,
                        const QString &score,
                        QWidget *parent = nullptr);
};