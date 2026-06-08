#include "MainWindow.h"
#include "ui/DisplayInterfaceWidget.h"
#include "ui/MatchResultWidget.h"
#include "ui/ClickStockWidget.h"
#include "model/KLineTime.h"
#include "data/DataBase.h"
#include "data/DataPath.h"
#include "alg/StockSelectorAlg.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QButtonGroup>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QProgressDialog>
#include <QApplication>

namespace
{
    const QString kDefaultCode = QStringLiteral("sh000001");
    const QString kDefaultName = QStringLiteral("上证指数");

    static void addShadow(QWidget *w, int blur = 28, int offsetY = 6)
    {
        auto *effect = new QGraphicsDropShadowEffect(w);
        effect->setBlurRadius(blur);
        effect->setOffset(0, offsetY);
        effect->setColor(QColor(120, 15, 25, 35));
        w->setGraphicsEffect(effect);
    }

    static QPushButton* createSideButton(const QString &text, bool checked = false)
    {
        auto *btn = new QPushButton(text);
        btn->setObjectName("SideButton");
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(52);
        return btn;
    }

    static ClickableStockChip *createClosableChip(const QString &text)
    {
        return new ClickableStockChip(text);
    }

    static QPushButton* createTimeButton(const QString &text, bool checked = false)
    {
        auto *btn = new QPushButton(text);
        btn->setObjectName("TimeButton");
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(44);
        return btn;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setObjectName("MainWindow");
    setWindowTitle("aiselectstock");
    resize(1200, 760);
    setMinimumSize(1000, 680);
    setAttribute(Qt::WA_StyledBackground, true);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);   // 去掉外围留白
    mainLayout->setSpacing(0);

    QWidget *sidebar = createSidebar();
    sidebar->setFixedWidth(150);
    mainLayout->addWidget(sidebar);

    auto *contentArea = new QWidget();
    contentArea->setAttribute(Qt::WA_StyledBackground, true);
    contentArea->setObjectName("ContentArea");


    auto *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(14);

    contentLayout->addWidget(createTopBar());
    createSearchPopup();
    contentLayout->addWidget(createStockTagsRow());

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName("BodySplitter");
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(6);

    QWidget *chartPanel = createChartPanel();
    QWidget *resultPanel = createResultPanel();

    splitter->addWidget(chartPanel);
    splitter->addWidget(resultPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({820, 280});

    contentLayout->addWidget(splitter, 1);

    mainLayout->addWidget(contentArea, 1);

    applyTheme();
    updateMaxButtonState();

    switchToSymbol(kDefaultCode, kDefaultName);
}

QWidget* MainWindow::createSidebar()
{
    auto *sidebar = new QWidget();
    sidebar->setObjectName("Sidebar");
    sidebar->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto *logoCard = new QWidget();
    logoCard->setObjectName("LogoCard");
    logoCard->setAttribute(Qt::WA_StyledBackground, true);

    auto *logoLayout = new QVBoxLayout(logoCard);
    logoLayout->setContentsMargins(14, 16, 14, 16);
    logoLayout->setSpacing(4);

    auto *logoMain = new QLabel("AI");
    logoMain->setStyleSheet("font: 700 34px 'Microsoft YaHei'; color: white;");

    auto *logoSub = new QLabel("aiselectstock");
    logoSub->setStyleSheet("font: 12px 'Microsoft YaHei'; color: rgba(255,255,255,0.88);");

    auto *logoDesc = new QLabel(QStringLiteral("走势匹配平台"));
    logoDesc->setStyleSheet("font: 10px 'Microsoft YaHei'; color: rgba(255,255,255,0.72);");

    logoLayout->addWidget(logoMain);
    logoLayout->addWidget(logoSub);
    logoLayout->addWidget(logoDesc);

    auto *mainSelectBtn = createSideButton(QStringLiteral("首页"), true);
    auto *aiSelectBtn     = createSideButton(QStringLiteral("AI选股"));

    auto *group = new QButtonGroup(sidebar);
    group->setExclusive(true);
    group->addButton(mainSelectBtn);
    group->addButton(aiSelectBtn);

    connect(aiSelectBtn, &QPushButton::clicked, this, [this, mainSelectBtn]()
            {
    showStockSelectDialog();
    if (mainSelectBtn) {
        mainSelectBtn->setChecked(true);
    } });

    auto *userCard = new QWidget();
    userCard->setObjectName("UserCard");
    userCard->setAttribute(Qt::WA_StyledBackground, true);

    auto *userLayout = new QVBoxLayout(userCard);
    userLayout->setContentsMargins(14, 14, 14, 14);
    userLayout->setSpacing(4);

    auto *userTitle = new QLabel(QStringLiteral("用户"));
    userTitle->setStyleSheet("font: 700 16px 'Microsoft YaHei'; color: white;");

    auto *userName = new QLabel(QStringLiteral("demo_user"));
    userName->setStyleSheet("font: 11px 'Microsoft YaHei'; color: rgba(255,255,255,0.85);");

    auto *userDesc = new QLabel(QStringLiteral("当前为界面演示版"));
    userDesc->setStyleSheet("font: 10px 'Microsoft YaHei'; color: rgba(255,255,255,0.70);");

    userLayout->addWidget(userTitle);
    userLayout->addWidget(userName);
    userLayout->addWidget(userDesc);

    layout->addWidget(logoCard);
    layout->addSpacing(8);
    layout->addWidget(mainSelectBtn);
    layout->addWidget(aiSelectBtn);
    layout->addStretch();
    layout->addWidget(userCard);

    return sidebar;
}

QWidget* MainWindow::createTopBar()
{
    auto *topBar = new QWidget();
    topBar->setObjectName("TopBar");

    m_topBar = topBar;
    m_topBar->installEventFilter(this);

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *searchWrap = new QWidget();
    searchWrap->setObjectName("SearchWrap");
    searchWrap->setAttribute(Qt::WA_StyledBackground, true);
    searchWrap->setFixedHeight(56);
    searchWrap->setMinimumWidth(520);
    searchWrap->setMaximumWidth(660);

    auto *searchLayout = new QHBoxLayout(searchWrap);
    searchLayout->setContentsMargins(14, 4, 4, 4);
    searchLayout->setSpacing(8);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setObjectName("SearchEdit");
    m_searchEdit->setPlaceholderText(QStringLiteral("输入股票名称 / 代码 "));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *searchBtn = new QPushButton(QStringLiteral("搜索"));
    searchBtn->setObjectName("SearchButton");
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setFixedWidth(110);
    searchBtn->setMinimumHeight(44);

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(searchBtn);

    m_syncDataBtn = new QPushButton(QStringLiteral("同步股票数据"));
    m_syncDataBtn->setObjectName("SyncDataButton");
    m_syncDataBtn->setCursor(Qt::PointingHandCursor);
    m_syncDataBtn->setFixedHeight(44);
    m_syncDataBtn->setMinimumWidth(132);
    m_syncDataBtn->setToolTip(QStringLiteral("调用 Python 脚本同步股票列表、指数和日K数据"));

    connect(m_syncDataBtn, &QPushButton::clicked, this, [this]()
            { startSyncMarketData(); });

    m_minBtn = new QPushButton(QStringLiteral("—"));
    m_minBtn->setObjectName("TitleButton");
    m_minBtn->setCursor(Qt::PointingHandCursor);
    m_minBtn->setFixedSize(48, 48);
    m_minBtn->setToolTip(QStringLiteral("最小化"));

    m_maxBtn = new QPushButton(QStringLiteral("□"));
    m_maxBtn->setObjectName("TitleButton");
    m_maxBtn->setCursor(Qt::PointingHandCursor);
    m_maxBtn->setFixedSize(48, 48);
    m_maxBtn->setToolTip(QStringLiteral("最大化"));

    m_closeBtn = new QPushButton(QStringLiteral("×"));
    m_closeBtn->setObjectName("CloseButton");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFixedSize(56, 56);
    m_closeBtn->setToolTip(QStringLiteral("关闭"));

    connect(m_minBtn, &QPushButton::clicked, this, &QWidget::showMinimized);

    connect(m_maxBtn, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        updateMaxButtonState();
    });

    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::close);

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]()
            { performSearch(); });

    connect(searchBtn, &QPushButton::clicked, this, [this]()
            {
    performSearch();

    if (!m_searchResultList || m_searchResultList->count() <= 0) {
        const QString keyword = m_searchEdit ? m_searchEdit->text().trimmed() : QString();

        if (!keyword.isEmpty()) {
            QMessageBox::information(
                this,
                QStringLiteral("搜索结果"),
                QStringLiteral("没有找到该股票，本软件只支持普通股。")
            );
        }

        return;
    }

    QListWidgetItem *item = m_searchResultList->currentItem();

    if (!item) {
        item = m_searchResultList->item(0);
    }

    onSearchResultClicked(item); });

    layout->addStretch(1);
    layout->addWidget(m_syncDataBtn);
    layout->addWidget(searchWrap);
    layout->addStretch(1);
    layout->addWidget(m_minBtn);
    layout->addWidget(m_maxBtn);
    layout->addWidget(m_closeBtn);

    return topBar;
}

void MainWindow::createSearchPopup()
{
    m_searchPopup = new QFrame(this);
    m_searchPopup->setObjectName("SearchPopup");
    m_searchPopup->setFrameShape(QFrame::NoFrame);

    auto *popupLayout = new QVBoxLayout(m_searchPopup);
    popupLayout->setContentsMargins(6, 6, 6, 6);
    popupLayout->setSpacing(0);

    m_searchResultList = new QListWidget(m_searchPopup);
    m_searchResultList->setObjectName("SearchResultList");
    m_searchResultList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_searchResultList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_searchResultList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_searchResultList->setMouseTracking(true);

    popupLayout->addWidget(m_searchResultList);

    m_searchPopup->resize(360, 220);
    m_searchPopup->hide();

    connect(m_searchResultList, &QListWidget::itemClicked,
            this, &MainWindow::onSearchResultClicked);
}

void MainWindow::updateSearchPopupPosition()
{
    if (!m_searchEdit || !m_searchPopup) {
        return;
    }

    const QPoint pos = m_searchEdit->mapTo(this, QPoint(0, m_searchEdit->height() + 6));
    m_searchPopup->setGeometry(pos.x(), pos.y(), m_searchEdit->width(), 220);
}

void MainWindow::hideSearchPopup()
{
    if (m_searchPopup) {
        m_searchPopup->hide();
    }
    if (m_searchResultList) {
        m_searchResultList->clear();
    }
}

void MainWindow::performSearch()
{
    if (!m_searchEdit || !m_searchResultList || !m_searchPopup) {
        return;
    }

    const QString keyword = m_searchEdit->text().trimmed();

    if (keyword.isEmpty()) {
        hideSearchPopup();
        return;
    }

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);
    const QVector<StockItem> items = repo.searchStocks(keyword, 20);

    showSearchResults(items);
}

void MainWindow::showSearchResults(const QVector<StockItem> &items)
{
    if (!m_searchPopup || !m_searchResultList) {
        return;
    }

    m_searchResultList->clear();

    if (items.isEmpty()) {
        m_searchPopup->hide();
        return;
    }

    for (const auto &item : items)
    {
        auto *listItem = new QListWidgetItem(
            QString("%1    %2").arg(item.code, item.name),
            m_searchResultList);

        listItem->setData(Qt::UserRole, item.code);
        listItem->setData(Qt::UserRole + 1, item.name);
    }

    updateSearchPopupPosition();
    m_searchPopup->show();
    m_searchPopup->raise();
}

void MainWindow::onSearchResultClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    const QString code = item->data(Qt::UserRole).toString();

    QString name = item->data(Qt::UserRole + 1).toString().trimmed();

    if (name.isEmpty()) {
        name = code;
    }

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);
    const QVector<KLineData> bars = repo.loadDailyBars(code);

    if (bars.isEmpty())
    {
        hideSearchPopup();

        QMessageBox::information(
            this,
            QStringLiteral("暂无数据"),
            QStringLiteral("该股票暂无 K 线数据，请先同步该股票数据。"));

        return;
    }

    if (m_searchEdit)
    {
        QSignalBlocker blocker(m_searchEdit);
        m_searchEdit->clear();
    }

    hideSearchPopup();

    ensureStockTabExists(code, name);
    switchToSymbol(code, name);
}

void MainWindow::ensureStockTabExists(const QString &code, const QString &name)
{
    if (!m_stockTagsLayout) {
        return;
    }

    for (int i = 0; i < m_stockTagsLayout->count(); ++i) {
        QWidget *w = m_stockTagsLayout->itemAt(i)->widget();
        auto *chip = qobject_cast<ClickableStockChip*>(w);
        if (!chip) {
            continue;
        }

        if (chip->property("code").toString() == code) {
            return;
        }
    }

    QString displayName = name.trimmed();

    if (displayName.isEmpty())
    {
        displayName = code;
    }

    auto *chip = createClosableChip(displayName);
    chip->setClosable(true);
    chip->setProperty("code", code);
    chip->setProperty("name", displayName);
    chip->setProperty("active", false);

    connect(chip, &ClickableStockChip::clicked, this, [this, code, displayName]()
            { switchToSymbol(code, displayName); });

    connect(chip, &ClickableStockChip::closeRequested, this, [this, code](ClickableStockChip *chip) {
        Q_UNUSED(chip);
        if (m_currentCode == code) {
            switchToSymbol(kDefaultCode, kDefaultName);
        }
    });

    int insertIndex = m_stockTagsLayout->count() - 1;
    if (insertIndex < 0) {
        insertIndex = 0;
    }

    m_stockTagsLayout->insertWidget(insertIndex, chip);

    chip->style()->unpolish(chip);
    chip->style()->polish(chip);
    chip->update();

    const auto children = chip->findChildren<QWidget *>();
    for (QWidget *child : children)
    {
        child->style()->unpolish(child);
        child->style()->polish(child);
        child->update();
    }
}

void MainWindow::switchToSymbol(const QString &code, const QString &name)
{
    const QString dbPath = AppPaths::databasePath();

    DataBase repo(dbPath);
    const QVector<KLineData> bars = repo.loadDailyBars(code);

    qDebug() << "[MainWindow] dbPath =" << dbPath;
    qDebug() << "[MainWindow] switchToSymbol code =" << code
             << ", name =" << name
             << ", bars =" << bars.size();

    if (bars.isEmpty())
    {
        qWarning() << "[MainWindow] no daily bars, code =" << code
                   << ", name =" << name
                   << ", dbPath =" << dbPath;
        return;
    }

    m_currentCode = code;

    if (m_chart)
    {
        m_chart->setSymbolInfo(code, name);
        m_chart->setDailyBars(bars);
    }

    setActiveTab(code);
}

void MainWindow::setActiveTab(const QString &code)
{
    if (!m_stockTagsLayout) {
        return;
    }

    for (int i = 0; i < m_stockTagsLayout->count(); ++i) {
        QWidget *w = m_stockTagsLayout->itemAt(i)->widget();
        auto *chip = qobject_cast<ClickableStockChip*>(w);
        if (!chip) {
            continue;
        }

        const bool active = (chip->property("code").toString() == code);
        chip->setProperty("active", active);

        chip->style()->unpolish(chip);
        chip->style()->polish(chip);
        chip->update();

        const auto children = chip->findChildren<QWidget *>();
        for (QWidget *child : children) {
            child->style()->unpolish(child);
            child->style()->polish(child);
            child->update();
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (m_searchPopup && m_searchPopup->isVisible()) {
        updateSearchPopupPosition();
    }
}

QWidget* MainWindow::createStockTagsRow()
{
    m_stockTagsRow = new QWidget();
    m_stockTagsLayout = new QHBoxLayout(m_stockTagsRow);
    m_stockTagsLayout->setContentsMargins(0, 0, 0, 0);
    m_stockTagsLayout->setSpacing(10);

    auto *indexChip = createClosableChip(kDefaultName);
    indexChip->setClosable(true);
    indexChip->setProperty("code", kDefaultCode);
    indexChip->setProperty("name", kDefaultName);
    indexChip->setProperty("active", true);

    connect(indexChip, &ClickableStockChip::clicked, this, [this]()
            { switchToSymbol(kDefaultCode, kDefaultName); });

    m_stockTagsLayout->addWidget(indexChip);
    m_stockTagsLayout->addStretch();

    return m_stockTagsRow;
}

QWidget* MainWindow::createChartPanel()
{
    auto *panel = new QWidget();
    panel->setObjectName("Panel");
    panel->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *timeRow = new QWidget();
    auto *timeLayout = new QHBoxLayout(timeRow);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(10);

    auto *timeGroup = new QButtonGroup(timeRow);
    timeGroup->setExclusive(true);

    auto *btnDay = createTimeButton(QStringLiteral("日K"), true);
    auto *btnWeek = createTimeButton(QStringLiteral("周K"));
    auto *btnMonth = createTimeButton(QStringLiteral("月K"));

    timeGroup->addButton(btnDay);
    timeGroup->addButton(btnWeek);
    timeGroup->addButton(btnMonth);

    timeLayout->addWidget(btnDay);
    timeLayout->addWidget(btnWeek);
    timeLayout->addWidget(btnMonth);
    timeLayout->addStretch();

    layout->addWidget(timeRow);

    m_chart = new DisplayInterfaceWidget();
    layout->addWidget(m_chart, 1);

    connect(btnDay, &QPushButton::clicked, this, [this]() {
        if (m_chart) m_chart->setPeriod(KLineTime::Daily);
    });
    connect(btnWeek, &QPushButton::clicked, this, [this]() {
        if (m_chart) m_chart->setPeriod(KLineTime::Weekly);
    });
    connect(btnMonth, &QPushButton::clicked, this, [this]() {
        if (m_chart) m_chart->setPeriod(KLineTime::Monthly);
    });

    auto *footerTip = new QLabel(QStringLiteral("说明：本软件仅用于走势筛选与可视化，不提供买卖操作。"));
    footerTip->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");
    layout->addWidget(footerTip);

    addShadow(panel, 28, 6);
    return panel;
}

QWidget* MainWindow::createResultPanel()
{
    auto *panel = new QWidget();
    panel->setObjectName("ResultPanel");
    panel->setAttribute(Qt::WA_StyledBackground, true);
    panel->setMinimumWidth(240);
    panel->setMaximumWidth(320);
    panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("选股结果"));
    title->setStyleSheet("font: 700 20px 'Microsoft YaHei'; color: #2d3138;");

    auto *filterBtn = new QPushButton(QStringLiteral("筛选"));
    filterBtn->setObjectName("FilterButton");
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setFixedSize(78, 34);

    connect(filterBtn, &QPushButton::clicked, this, [this]() {
        showResultFilterDialog();
    });

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(filterBtn);

    auto *subTitle = new QLabel(QStringLiteral("根据你的走势条件筛出的候选票"));
    subTitle->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #8b909a;");

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget();
    m_resultListLayout = new QVBoxLayout(container);
    m_resultListLayout->setContentsMargins(0, 0, 0, 0);
    m_resultListLayout->setSpacing(12);
    m_resultListLayout->addStretch();

    scrollArea->setWidget(container);

    auto *bottomInfo = new QLabel(QStringLiteral("默认按匹配度从高到低排序"));
    bottomInfo->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");

    layout->addLayout(headerRow);
    layout->addWidget(subTitle);
    layout->addWidget(scrollArea, 1);
    layout->addWidget(bottomInfo);

    initDefaultMatchResults();
    sortMatchResults();
    refreshMatchResults();

    addShadow(panel, 28, 6);
    return panel;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void MainWindow::updateMaxButtonState()
{
    if (!m_maxBtn) {
        return;
    }

    if (isMaximized()) {
        m_maxBtn->setText(QStringLiteral("❐"));
        m_maxBtn->setToolTip(QStringLiteral("还原"));
    } else {
        m_maxBtn->setText(QStringLiteral("□"));
        m_maxBtn->setToolTip(QStringLiteral("最大化"));
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_topBar && event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (isMaximized()) {
                showNormal();
            } else {
                showMaximized();
            }
            updateMaxButtonState();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void MainWindow::initDefaultMatchResults()
{
    m_matchResults.clear();

    struct SeedResult
    {
        QString name;
        QString code;
        double score;
    };

    const QVector<SeedResult> seeds = {
        // {QStringLiteral("平安银行"), QStringLiteral("000001"), 92.0},
        // {QStringLiteral("万科A"), QStringLiteral("000002"), 88.0},
        // {QStringLiteral("深振业A"), QStringLiteral("000006"), 85.0},
        // {QStringLiteral("全新好"), QStringLiteral("000007"), 99.0},
        // {QStringLiteral("神州高铁"), QStringLiteral("000008"), 97.0},
        // {QStringLiteral("国药一致"), QStringLiteral("000028"), 97.0},
    };

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);

    for (const auto &seed : seeds)
    {
        MatchStockResult result;
        result.name = seed.name;
        result.code = seed.code;
        result.score = seed.score;

        const QVector<KLineData> bars = repo.loadDailyBars(seed.code);
        if (!bars.isEmpty())
        {
            const KLineData last = bars.last();
            result.price = last.close;
            result.volume = last.volume;
            result.amount = last.amount;
            result.turnover = last.turnover;
        }

        m_matchResults.push_back(result);
    }
}

void MainWindow::sortMatchResults()
{
    std::sort(m_matchResults.begin(), m_matchResults.end(),
              [this](const MatchStockResult &a, const MatchStockResult &b) {
        double left = 0.0;
        double right = 0.0;

        switch (m_resultSortField)
        {
        case ResultSortField::MatchScore:
            left = a.score;
            right = b.score;
            break;
        case ResultSortField::Price:
            left = a.price;
            right = b.price;
            break;
        case ResultSortField::Volume:
            left = a.volume;
            right = b.volume;
            break;
        case ResultSortField::Amount:
            left = a.amount;
            right = b.amount;
            break;
        case ResultSortField::Turnover:
            left = a.turnover;
            right = b.turnover;
            break;
        }

        if (m_resultSortAscending) {
            return left < right;
        }

        return left > right;
    });
}

void MainWindow::refreshMatchResults()
{
    if (!m_resultListLayout) {
        return;
    }

    while (m_resultListLayout->count() > 0)
    {
        QLayoutItem *item = m_resultListLayout->takeAt(0);

        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }

        delete item;
    }

    for (const auto &result : m_matchResults)
    {
        auto *card = new MatchResultWidget(
            result.name,
            result.code,
            QString::number(result.score, 'f', 0),
            formatPrice(result.price),
            formatVolume(result.volume),
            formatAmount(result.amount),
            formatTurnover(result.turnover)
        );

        connect(card, &MatchResultWidget::viewRequested, this,
                [this](const QString &code, const QString &name) {
            ensureStockTabExists(code, name);
            switchToSymbol(code, name);
        });

        m_resultListLayout->addWidget(card);
    }

    m_resultListLayout->addStretch();
}

void MainWindow::showResultFilterDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("筛选选股结果"));
    dialog.setModal(true);
    dialog.resize(260, 140);

    auto *layout = new QVBoxLayout(&dialog);

    auto *formLayout = new QFormLayout();

    auto *fieldBox = new QComboBox(&dialog);
    fieldBox->addItem(QStringLiteral("匹配度"), static_cast<int>(ResultSortField::MatchScore));
    fieldBox->addItem(QStringLiteral("股票价格"), static_cast<int>(ResultSortField::Price));
    fieldBox->addItem(QStringLiteral("成交量"), static_cast<int>(ResultSortField::Volume));
    fieldBox->addItem(QStringLiteral("成交额"), static_cast<int>(ResultSortField::Amount));
    fieldBox->addItem(QStringLiteral("换手率"), static_cast<int>(ResultSortField::Turnover));

    auto *orderBox = new QComboBox(&dialog);
    orderBox->addItem(QStringLiteral("从高到低"), false);
    orderBox->addItem(QStringLiteral("从低到高"), true);

    fieldBox->setCurrentIndex(static_cast<int>(m_resultSortField));
    orderBox->setCurrentIndex(m_resultSortAscending ? 1 : 0);

    formLayout->addRow(QStringLiteral("排序字段："), fieldBox);
    formLayout->addRow(QStringLiteral("排序方式："), orderBox);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );

    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_resultSortField = static_cast<ResultSortField>(
        fieldBox->currentData().toInt()
    );

    m_resultSortAscending = orderBox->currentData().toBool();

    sortMatchResults();
    refreshMatchResults();
}

void MainWindow::showStockSelectDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("选股方式"));
    dialog.setModal(true);
    dialog.resize(360, 230);
    dialog.setObjectName("StockSelectDialog");

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("请选择选股方式"), &dialog);
    title->setStyleSheet("font: 700 20px 'Microsoft YaHei'; color: #2d3138;");

    auto *desc = new QLabel(
        QStringLiteral("这里先保留 AI选股 和 传统选股 两个接口，后面可以直接接入你的真实算法。"),
        &dialog
    );
    desc->setWordWrap(true);
    desc->setStyleSheet("font: 12px 'Microsoft YaHei'; color: #7d838e;");

    auto *aiRadio = new QRadioButton(QStringLiteral("AI选股"), &dialog);
    auto *traditionalRadio = new QRadioButton(QStringLiteral("传统选股"), &dialog);
    aiRadio->setChecked(true);

    aiRadio->setStyleSheet("font: 600 15px 'Microsoft YaHei'; color: #2d3138;");
    traditionalRadio->setStyleSheet("font: 600 15px 'Microsoft YaHei'; color: #2d3138;");

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog
    );

    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("开始选股"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(title);
    layout->addWidget(desc);
    layout->addSpacing(4);
    layout->addWidget(aiRadio);
    layout->addWidget(traditionalRadio);
    layout->addStretch();
    layout->addWidget(buttons);

    dialog.setStyleSheet(R"(
        QDialog#StockSelectDialog {
            background: #fffafa;
        }
        QRadioButton::indicator {
            width: 16px;
            height: 16px;
        }
        QPushButton {
            min-width: 82px;
            min-height: 30px;
            border-radius: 15px;
            padding: 0 14px;
            font: 600 12px "Microsoft YaHei";
            background: #fff3f5;
            color: #bc1d2c;
            border: 1px solid #efc1c8;
        }
        QPushButton:hover {
            background: #ffe8eb;
        }
    )");

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const StockSelectMode mode = aiRadio->isChecked()
        ? StockSelectMode::Ai
        : StockSelectMode::Traditional;

    runStockSelect(mode);
}

void MainWindow::runStockSelect(StockSelectMode mode)
{
    const QString title = mode == StockSelectMode::Ai
        ? QStringLiteral("AI选股")
        : QStringLiteral("传统选股");

    QProgressDialog progressDialog(this);
    progressDialog.setWindowTitle(QStringLiteral("选股中"));
    progressDialog.setLabelText(QStringLiteral("正在准备选股数据..."));
    progressDialog.setCancelButton(nullptr);
    progressDialog.setWindowModality(Qt::ApplicationModal);
    progressDialog.setMinimumDuration(0);
    progressDialog.setAutoClose(false);
    progressDialog.setAutoReset(false);
    progressDialog.setRange(0, 100);
    progressDialog.setValue(0);
    progressDialog.resize(420, 120);
    progressDialog.show();

    QApplication::processEvents();

    if (mode == StockSelectMode::Ai) {
        m_matchResults = requestAiSelectResults(&progressDialog);
    } else {
        m_matchResults = requestTraditionalSelectResults(&progressDialog);
    }

    progressDialog.setLabelText(QStringLiteral("选股完成，正在刷新结果..."));
    progressDialog.setValue(progressDialog.maximum());
    QApplication::processEvents();

    progressDialog.close();

    sortMatchResults();
    refreshMatchResults();

    if (m_matchResults.isEmpty()) {
        QMessageBox::information(this,
                                 title,
                                 QStringLiteral("暂无符合条件的股票。请先同步股票数据，或者适当放宽选股条件。"));
    }
}

QVector<MainWindow::MatchStockResult> MainWindow::requestAiSelectResults(QProgressDialog *progressDialog)
{
    QVector<MatchStockResult> results;

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);

    const QVector<StockItem> stocks = repo.loadAllStocks();

    const int total = static_cast<int>(stocks.size());

    if (progressDialog) {
        progressDialog->setRange(0, total);
        progressDialog->setValue(0);
        progressDialog->setLabelText(QStringLiteral("正在进行 AI选股..."));
        QApplication::processEvents();
    }

    QVector<StockSelectAlgResult> algResults;

    for (int i = 0; i < total; ++i)
    {
        const StockItem &stock = stocks[i];

        if (progressDialog) {
            progressDialog->setValue(i);
            progressDialog->setLabelText(QStringLiteral("AI选股中：%1 / %2\n正在分析：%3 %4")
                                             .arg(i + 1)
                                             .arg(total)
                                             .arg(stock.code)
                                             .arg(stock.name));
            QApplication::processEvents();
        }

        const QVector<KLineData> bars = repo.loadDailyBars(stock.code);

        StockSelectAlgResult algResult;

        if (StockSelectorAlg::evaluateAiStock(stock, bars, &algResult)) {
            algResults.push_back(algResult);
        }
    }

    if (progressDialog) {
        progressDialog->setValue(total);
        progressDialog->setLabelText(QStringLiteral("AI选股计算完成，正在排序..."));
        QApplication::processEvents();
    }

    algResults = StockSelectorAlg::sortAndLimit(algResults, 20);

    for (const StockSelectAlgResult &item : algResults)
    {
        MatchStockResult result;
        result.code = item.code;
        result.name = item.name;
        result.score = item.score;
        result.price = item.price;
        result.volume = item.volume;
        result.amount = item.amount;
        result.turnover = item.turnover;

        results.push_back(result);
    }

    return results;
}

QVector<MainWindow::MatchStockResult> MainWindow::requestTraditionalSelectResults(QProgressDialog *progressDialog)
{
    QVector<MatchStockResult> results;

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);

    const QVector<StockItem> stocks = repo.loadAllStocks();

    const int total = static_cast<int>(stocks.size());

    if (progressDialog) {
        progressDialog->setRange(0, total);
        progressDialog->setValue(0);
        progressDialog->setLabelText(QStringLiteral("正在进行传统选股..."));
        QApplication::processEvents();
    }

    QVector<StockSelectAlgResult> algResults;

    for (int i = 0; i < total; ++i)
    {
        const StockItem &stock = stocks[i];

        if (progressDialog) {
            progressDialog->setValue(i);
            progressDialog->setLabelText(QStringLiteral("传统选股中：%1 / %2\n正在分析：%3 %4")
                                             .arg(i + 1)
                                             .arg(total)
                                             .arg(stock.code)
                                             .arg(stock.name));
            QApplication::processEvents();
        }

        const QVector<KLineData> bars = repo.loadDailyBars(stock.code);

        StockSelectAlgResult algResult;

        if (StockSelectorAlg::evaluateTraditionalStock(stock, bars, &algResult)) {
            algResults.push_back(algResult);
        }
    }

    if (progressDialog) {
        progressDialog->setValue(total);
        progressDialog->setLabelText(QStringLiteral("传统选股计算完成，正在排序..."));
        QApplication::processEvents();
    }

    algResults = StockSelectorAlg::sortAndLimit(algResults, 20);

    for (const StockSelectAlgResult &item : algResults)
    {
        MatchStockResult result;
        result.code = item.code;
        result.name = item.name;
        result.score = item.score;
        result.price = item.price;
        result.volume = item.volume;
        result.amount = item.amount;
        result.turnover = item.turnover;

        results.push_back(result);
    }

    return results;
}

MainWindow::MatchStockResult MainWindow::buildMatchResult(const QString &code,
                                                          const QString &name,
                                                          double score) const
{
    MatchStockResult result;
    result.code = code;
    result.name = name;
    result.score = score;

    const QString dbPath = AppPaths::databasePath();
    DataBase repo(dbPath);
    const QVector<KLineData> bars = repo.loadDailyBars(code);

    if (!bars.isEmpty()) {
        const KLineData last = bars.last();
        result.price = last.close;
        result.volume = last.volume;
        result.amount = last.amount;
        result.turnover = last.turnover;
    }

    return result;
}

void MainWindow::startSyncMarketData()
{
    if (m_syncProcess && m_syncProcess->state() != QProcess::NotRunning) {
        QMessageBox::information(this,
                                 QStringLiteral("正在同步"),
                                 QStringLiteral("股票数据正在同步中，请等待当前任务完成。"));
        return;
    }

    const QString scriptPath = findSyncMarketDataScript();
    if (scriptPath.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("脚本不存在"),
                             QStringLiteral("没有找到 tools/sync_market_data.py，请确认 tools 目录在项目目录下。"));
        return;
    }

    m_syncOutputBuffer.clear();
    m_syncErrorBuffer.clear();

    m_syncProgress = new QProgressDialog(QStringLiteral("正在准备同步股票数据..."),
                                         QStringLiteral("取消"),
                                         0,
                                         0,
                                         this);
    m_syncProgress->setWindowTitle(QStringLiteral("同步股票数据"));
    m_syncProgress->setWindowModality(Qt::WindowModal);
    m_syncProgress->setMinimumDuration(0);
    m_syncProgress->setAutoClose(false);
    m_syncProgress->setAutoReset(false);
    m_syncProgress->resize(420, 120);
    m_syncProgress->show();

    m_syncProcess = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_syncProcess->setProcessEnvironment(env);

    const QFileInfo scriptInfo(scriptPath);
    m_syncProcess->setWorkingDirectory(scriptInfo.absolutePath());

    connect(m_syncProgress, &QProgressDialog::canceled, this, [this]() {
        if (m_syncProcess && m_syncProcess->state() != QProcess::NotRunning) {
            m_syncProgress->setLabelText(QStringLiteral("正在取消同步..."));
            m_syncProcess->kill();
        }
    });

    connect(m_syncProcess, &QProcess::readyReadStandardOutput,
            this, &MainWindow::handleSyncProcessOutput);

    connect(m_syncProcess, &QProcess::readyReadStandardError,
            this, &MainWindow::handleSyncProcessErrorOutput);

    connect(m_syncProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        finishSyncMarketData(exitCode, static_cast<int>(exitStatus));
    });

    if (m_syncDataBtn) {
        m_syncDataBtn->setEnabled(false);
    }

#ifdef Q_OS_WIN
    const QString program = QStringLiteral("python");
#else
    const QString program = QStringLiteral("python3");
#endif

    QStringList args;
    args << QStringLiteral("-u")
         << scriptPath
         << QStringLiteral("--workers")
         << QStringLiteral("6");

    m_syncProcess->start(program, args);

    if (!m_syncProcess->waitForStarted(3000)) {
        if (m_syncDataBtn) {
            m_syncDataBtn->setEnabled(true);
        }

        if (m_syncProgress) {
            m_syncProgress->close();
            m_syncProgress->deleteLater();
            m_syncProgress = nullptr;
        }

        m_syncProcess->deleteLater();
        m_syncProcess = nullptr;

        QMessageBox::warning(this,
                             QStringLiteral("启动失败"),
                             QStringLiteral("无法启动 Python。请确认已经安装 Python，并且 python 命令已加入 PATH。"));
    }
}

void MainWindow::handleSyncProcessOutput()
{
    if (!m_syncProcess) {
        return;
    }

    m_syncOutputBuffer += QString::fromUtf8(m_syncProcess->readAllStandardOutput());

    while (true) {
        const int newlineIndex = m_syncOutputBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        QString line = m_syncOutputBuffer.left(newlineIndex).trimmed();
        m_syncOutputBuffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        qDebug() << "[sync stdout]" << line;
        handleSyncProgressLine(line);
    }
}

void MainWindow::handleSyncProcessErrorOutput()
{
    if (!m_syncProcess) {
        return;
    }

    m_syncErrorBuffer += QString::fromUtf8(m_syncProcess->readAllStandardError());

    while (true) {
        const int newlineIndex = m_syncErrorBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        QString line = m_syncErrorBuffer.left(newlineIndex).trimmed();
        m_syncErrorBuffer.remove(0, newlineIndex + 1);

        if (!line.isEmpty()) {
            qWarning() << "[sync stderr]" << line;
        }
    }
}

void MainWindow::handleSyncProgressLine(const QString &line)
{
    if (!m_syncProgress) {
        return;
    }

    if (line.startsWith(QStringLiteral("PHASE|"))) {
        const QStringList parts = line.split('|');
        if (parts.size() >= 2) {
            m_syncProgress->setLabelText(parts.mid(1).join(QStringLiteral(" | ")));
        }
        return;
    }

    if (!line.startsWith(QStringLiteral("PROGRESS|"))) {
        return;
    }

    const QStringList parts = line.split('|');
    if (parts.size() < 6) {
        return;
    }

    const int current = parts.value(1).toInt();
    const int total = parts.value(2).toInt();
    const QString code = parts.value(3);
    const QString status = parts.value(4);
    const QString message = parts.mid(5).join(QStringLiteral(" | "));

    if (total > 0 && m_syncProgress->maximum() != total) {
        m_syncProgress->setRange(0, total);
    }

    m_syncProgress->setValue(current);

    QString statusText;
    if (status == QStringLiteral("success"))
    {
        statusText = QStringLiteral("成功");
    }
    else if (status == QStringLiteral("failed"))
    {
        statusText = QStringLiteral("失败");
    }
    else
    {
        statusText = status;
    }

    QString displayMessage = message;
    if (status == QStringLiteral("failed"))
    {
        displayMessage = QStringLiteral("同步失败，已跳过，继续同步下一只股票");
    }

    m_syncProgress->setLabelText(
        QStringLiteral("正在同步股票数据：%1 / %2\n当前：%3，状态：%4\n%5")
            .arg(current)
            .arg(total)
            .arg(code)
            .arg(statusText)
            .arg(displayMessage)
    );
}

void MainWindow::refreshCurrentSymbolAfterSync()
{
    QString code = m_currentCode.trimmed();

    if (code.isEmpty()) {
        code = kDefaultCode;
    }

    QString name = (code == kDefaultCode) ? kDefaultName : code;

    if (m_stockTagsLayout) {
        for (int i = 0; i < m_stockTagsLayout->count(); ++i) {
            QWidget *w = m_stockTagsLayout->itemAt(i)->widget();
            auto *chip = qobject_cast<ClickableStockChip *>(w);

            if (!chip) {
                continue;
            }

            if (chip->property("code").toString() == code) {
                const QString chipName = chip->property("name").toString().trimmed();
                if (!chipName.isEmpty()) {
                    name = chipName;
                }
                break;
            }
        }
    }

    switchToSymbol(code, name);
}

void MainWindow::finishSyncMarketData(int exitCode, int exitStatus)
{
    Q_UNUSED(exitStatus);

    if (m_syncDataBtn) {
        m_syncDataBtn->setEnabled(true);
    }

    const bool ok = (exitCode == 0);

    if (m_syncProgress) {
        if (m_syncProgress->maximum() > 0) {
            m_syncProgress->setValue(m_syncProgress->maximum());
        }

        m_syncProgress->close();
        m_syncProgress->deleteLater();
        m_syncProgress = nullptr;
    }

    if (m_syncProcess) {
        m_syncProcess->deleteLater();
        m_syncProcess = nullptr;
    }

    if (ok) {
    initDefaultMatchResults();
    sortMatchResults();
    refreshMatchResults();

    refreshCurrentSymbolAfterSync();

    QCoreApplication::processEvents();

    QMessageBox::information(this,
                             QStringLiteral("同步完成"),
                             QStringLiteral("股票数据同步完成，界面已刷新。"));
}
}

QString MainWindow::findSyncMarketDataScript() const
{
    const QString appDir = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        QDir(appDir).filePath("../../../../tools/sync_market_data.py"),
        QDir(appDir).filePath("../../../tools/sync_market_data.py"),
        QDir(appDir).filePath("../../tools/sync_market_data.py"),
        QDir(appDir).filePath("../tools/sync_market_data.py"),
        QDir(appDir).filePath("tools/sync_market_data.py"),
        QDir::current().filePath("tools/sync_market_data.py")
    };

    for (const QString &path : candidates) {
        const QString cleanPath = QDir::cleanPath(path);
        if (QFileInfo::exists(cleanPath)) {
            return cleanPath;
        }
    }

    return QString();
}

QString MainWindow::formatPrice(double value) const
{
    if (value <= 0.0) {
        return QStringLiteral("--");
    }

    return QString::number(value, 'f', 2);
}

QString MainWindow::formatVolume(double value) const
{
    if (value <= 0.0) {
        return QStringLiteral("--");
    }

    return QStringLiteral("%1万手").arg(QString::number(value / 10000.0, 'f', 2));
}

QString MainWindow::formatAmount(double value) const
{
    if (value <= 0.0) {
        return QStringLiteral("--");
    }

    return QStringLiteral("%1亿").arg(QString::number(value / 100000000.0, 'f', 2));
}

QString MainWindow::formatTurnover(double value) const
{
    if (value <= 0.0) {
        return QStringLiteral("--");
    }

    return QStringLiteral("%1%").arg(QString::number(value, 'f', 2));
}

void MainWindow::applyTheme()
{
    setStyleSheet(R"(
        QWidget#MainWindow {
            background: #fffafa;
        }

        QWidget#ContentArea {
           background: #fffafa;
        }

        QWidget#Sidebar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                        stop:0 #781019,
                                        stop:0.55 #b91524,
                                        stop:1 #d62839);
            border: none;
            border-radius: 0px;
        }

        QWidget#LogoCard, QWidget#UserCard {
            background: rgba(255, 255, 255, 0.10);
            border: 1px solid rgba(255, 255, 255, 0.18);
            border-radius: 18px;
        }

        QPushButton#SideButton {
            background: transparent;
            border: none;
            border-radius: 14px;
            color: rgba(255,255,255,0.90);
            text-align: left;
            padding-left: 18px;
            font: 600 18px "Microsoft YaHei";
        }

        QPushButton#SideButton:hover {
            background: rgba(255,255,255,0.10);
        }

        QPushButton#SideButton:checked {
            background: rgba(255,255,255,0.18);
            color: white;
        }

        QPushButton#SyncDataButton {
            background: #d62839;
            border: none;
            border-radius: 18px;
            color: white;
            padding: 0 16px;
            font: 700 14px "Microsoft YaHei";
        }

        QPushButton#SyncDataButton:hover {
            background: #bf1f30;
        }

        QPushButton#SyncDataButton:disabled {
            background: #d8a5ab;
            color: rgba(255,255,255,0.75);
        }

        QWidget#SearchWrap {
            background: white;
            border: 2px solid #f0c9ce;
            border-radius: 18px;
        }

        QLineEdit#SearchEdit {
            border: none;
            background: transparent;
            color: #2e3138;
            padding-left: 6px;
            font: 15px "Microsoft YaHei";
        }

        QLineEdit#SearchEdit::placeholder {
            color: #a0a4ac;
        }

        QPushButton#SearchButton {
            background: #d62839;
            color: white;
            border: none;
            border-top-right-radius: 14px;
            border-bottom-right-radius: 14px;
            border-top-left-radius: 10px;
            border-bottom-left-radius: 10px;
            padding: 0 22px;
            font: 700 15px "Microsoft YaHei";
        }

        QPushButton#SearchButton:hover {
            background: #be1f2f;
        }

        QPushButton#SearchButton:pressed {
            background: #a91927;
        }

        QFrame#SearchPopup {
            background: white;
            border: 1px solid #d9dde5;
            border-radius: 12px;
        }

        QListWidget#SearchResultList {
            border: none;
            background: transparent;
            outline: none;
            color: #2e3138;
            font: 13px "Microsoft YaHei";
            padding: 4px;
        }

        QListWidget#SearchResultList::item {
            height: 34px;
            padding: 4px 10px;
            border-radius: 6px;
        }

        QListWidget#SearchResultList::item:hover {
            background: #f6f8fb;
        }

        QListWidget#SearchResultList::item:selected {
            background: #e9f1ff;
            color: #1f2d3d;
        }

        QPushButton#TitleButton {
            background: #fff1f3;
            color: #c61f2f;
            border: 1px solid #f1c3ca;
            border-radius: 14px;
            font: 700 18px "Microsoft YaHei"; 
       }

        QPushButton#TitleButton:hover {
            background: #ffe3e7;
        }

        QPushButton#CloseButton {
            background: #fff1f3;
            color: #c61f2f;
            border: 1px solid #f1c3ca;
            border-radius: 16px;
            font: 700 28px "Microsoft YaHei";
        }

        QPushButton#CloseButton:hover {
            background: #ffe3e7;
        }

        QWidget#StockChip {
            background: white;
            border: 1px solid #d62839;
            border-radius: 12px;
        }

        QWidget#StockChip:hover {
            background: #fff5f6;
        }

        QWidget#StockChip[active="true"] {
            background: #d62839;
            border: 1px solid #d62839;
        }

        QWidget#StockChip QLabel#StockChipLabel {
            color: #d62839;
            font: 13px "Microsoft YaHei";
            background: transparent;
            border: none;
        }

        QWidget#StockChip[active="true"] QLabel#StockChipLabel {
            color: white;
            font: 700 13px "Microsoft YaHei";
            background: transparent;
            border: none;
        }

        QWidget#StockChip QToolButton#StockChipClose {
            color: #d62839;
            border: none;
            background: transparent;
            font: 13px "Microsoft YaHei";
            padding: 0;
        }

        QWidget#StockChip QToolButton#StockChipClose:hover {
            color: #c62828;
        }

        QWidget#StockChip[active="true"] QToolButton#StockChipClose {
            color: white;
            border: none;
            background: transparent;
        }

        QWidget#StockChip[active="true"] QToolButton#StockChipClose:hover {
            color: #ffe6e6;
        }

        QWidget#Panel, QWidget#ResultPanel {
            background: white;
            border: 1px solid #f0d8dc;
            border-radius: 20px;
        }

        QPushButton#TimeButton {
            background: #fff4f5;
            color: #8f2430;
            border: 1px solid #efc3c8;
            border-radius: 14px;
            padding: 0 18px;
            font: 700 14px "Microsoft YaHei";
            min-width: 74px;
        }

        QPushButton#TimeButton:hover {
            background: #ffe6e9;
        }

        QPushButton#TimeButton:checked {
            background: #d62839;
            color: white;
            border: 1px solid #d62839;
        }

        QPushButton#FilterButton {
            background: #fff3f5;
            color: #bc1d2c;
            border: 1px solid #efc1c8;
            border-radius: 17px;
            font: 700 12px "Microsoft YaHei";
        }

        QPushButton#FilterButton:hover {
            background: #ffe8eb;
        }

        QFrame#ResultCard {
            background: #ffffff;
            border: 1px solid #f1dde0;
            border-radius: 18px;
        }

        QScrollArea {
            background: transparent;
            border: none;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 4px 0 4px 0;
        }

        QScrollBar::handle:vertical {
            background: #efc5cb;
            border-radius: 5px;
            min-height: 24px;
        }

        QScrollBar::handle:vertical:hover {
            background: #e59aa5;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0px;
        }
    )");
}