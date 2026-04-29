#pragma once

#include <QWidget>

class QLabel;
class QToolButton;

class ClickableStockChip : public QWidget
{
    Q_OBJECT

public:
    explicit ClickableStockChip(const QString &text, QWidget *parent = nullptr);

    QString text() const;
    void setClosable(bool closable);

signals:
    void clicked();
    void closeRequested(ClickableStockChip *chip);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_label = nullptr;
    QToolButton *m_closeBtn = nullptr;
};
