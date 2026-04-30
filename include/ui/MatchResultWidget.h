#pragma once

#include <QFrame>
#include <QString>

class MatchResultWidget : public QFrame
{
    Q_OBJECT
public:
    explicit MatchResultWidget(const QString &name,
                        const QString &code,
                        const QString &score,
                        QWidget *parent = nullptr);

    signals:
    void viewRequested(const QString &code, const QString &name);
};