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