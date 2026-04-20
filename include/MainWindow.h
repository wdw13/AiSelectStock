#pragma once

#include <QWidget>
#include <QPoint>

class QPushButton;
class QEvent;
class TrendPreviewWidget;

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
    TrendPreviewWidget *m_chart = nullptr;

    QPushButton *m_minBtn = nullptr;
    QPushButton *m_maxBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_topBar = nullptr;
};