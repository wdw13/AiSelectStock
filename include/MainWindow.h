#pragma once

#include <QWidget>
#include <QPoint>
#include <QFrame>
#include <QHBoxLayout>
#include "data/DataBase.h"

class QPushButton;
class QEvent;
class DisplayInterfaceWidget;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget* createSidebar();
    QWidget* createTopBar();
    QWidget* createStockTagsRow();
    QWidget* createChartPanel();
    QWidget* createResultPanel();
    void performSearch();
    void showSearchResults(const QVector<StockItem> &items);
    void onSearchResultClicked(QListWidgetItem *item);
    void updateSearchPopupPosition();
    void hideSearchPopup();

    void switchToSymbol(const QString &code, const QString &name);
    void ensureStockTabExists(const QString &code, const QString &name);
    void setActiveTab(const QString &code);
    void createSearchPopup();
    void applyTheme();
    void updateMaxButtonState();

private:
    bool m_dragging = false;
    QPoint m_dragPosition;
    DisplayInterfaceWidget *m_chart = nullptr;
    QWidget *m_stockTagsRow = nullptr;
    QHBoxLayout *m_stockTagsLayout = nullptr;

    QPushButton *m_minBtn = nullptr;
    QPushButton *m_maxBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_topBar = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_searchResultList = nullptr;
    QFrame *m_searchPopup = nullptr;
    QString m_currentCode;
};