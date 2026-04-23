#pragma once

#include <QWidget>
#include <QPoint>
#include <QRectF>
#include <QVector>

#include "model/KLineBar.h"
#include "model/KLinePeriod.h"

class QPaintEvent;
class QWheelEvent;
class QMouseEvent;

class TrendPreviewWidget : public QWidget
{
public:
    explicit TrendPreviewWidget(QWidget *parent = nullptr);

    void setDailyBars(const QVector<KLineBar>& bars);
    void setPeriod(KLinePeriod period);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF chartRect() const;
    void clampRightIndex();
    const QVector<KLineBar>& currentBars() const;

private:
    QVector<KLineBar> m_dailyBars;
    QVector<KLineBar> m_weeklyBars;
    QVector<KLineBar> m_monthlyBars;

    KLinePeriod m_period = KLinePeriod::Daily;

    int m_visibleBars = 80;    // 初始80格
    int m_rightIndex = -1;     // 右边最后一根K线索引

    double m_panOffsetPx = 0.0;
    bool m_panning = false;
    QPoint m_lastMousePos;
};