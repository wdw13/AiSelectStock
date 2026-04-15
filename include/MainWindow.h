#pragma once

#include <QFrame>
#include <QWidget>
#include <QString>
#include <QPushButton>

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

class ResultCard : public QFrame
{
public:
    explicit ResultCard(const QString &name,
                        const QString &code,
                        const QString &score,
                        QWidget *parent = nullptr);
};

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget* createSidebar();
    QWidget* createTopBar();
    QWidget* createStockTagsRow();
    QWidget* createChartPanel();
    QWidget* createResultPanel();
    void applyTheme();
    void updateMaxButtonState();

private:
    bool m_dragging = false;
    QPoint m_dragPosition;

    QPushButton *m_minBtn = nullptr;
    QPushButton *m_maxBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_topBar = nullptr;
};