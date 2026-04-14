#pragma once

#include <QFrame>
#include <QWidget>
#include <QString>

class TrendPreviewWidget : public QWidget
{
public:
    explicit TrendPreviewWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

class ResultCard : public QFrame
{
public:
    explicit ResultCard(const QString &name,
                        const QString &code,
                        const QString &rule,
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

private:
    QWidget* createSidebar();
    QWidget* createTopBar();
    QWidget* createStockTagsRow();
    QWidget* createChartPanel();
    QWidget* createResultPanel();
    void applyTheme();

    bool m_dragging = false;
    QPoint m_dragPosition;
};