#pragma once

#include <QWidget>
#include <QPoint>
#include <QRectF>

class QPaintEvent;
class QWheelEvent;
class QMouseEvent;

class TrendPreviewWidget : public QWidget
{
public:
    explicit TrendPreviewWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRectF chartRect() const;
    void clampXOffset(const QRectF &plot);

private:
    double m_xScale = 1.0;   // 只控制 X 轴缩放
    double m_xOffset = 0.0;  // 只控制 X 轴平移
    bool m_panning = false;
    QPoint m_lastMousePos;
};