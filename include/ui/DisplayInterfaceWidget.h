#pragma once

#include <QWidget>
#include <QPoint>
#include <QRectF>
#include <QVector>

#include "model/KLineData.h"
#include "model/KLineTime.h"

class QPaintEvent;
class QWheelEvent;
class QMouseEvent;

class DisplayInterfaceWidget : public QWidget
{
public:
    explicit DisplayInterfaceWidget(QWidget *parent = nullptr);

    void setDailyBars(const QVector<KLineData>& bars);
    void setPeriod(KLineTime period);
    void setSymbolInfo(const QString& code, const QString& name);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF chartRect() const;
    void clampRightIndex();
    const QVector<KLineData>& currentBars() const;

private:
    QVector<KLineData> m_dailyBars;
    QVector<KLineData> m_weeklyBars;
    QVector<KLineData> m_monthlyBars;

    KLineTime m_period = KLineTime::Daily;

    int m_visibleBars = 80;    // 初始80格
    int m_rightIndex = -1;     // 右边最后一根K线索引

    double m_panOffsetPx = 0.0;
    bool m_panning = false;
    QPoint m_lastMousePos;
    QString m_symbolCode;
    QString m_symbolName;
};