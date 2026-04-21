#include "MainWindow.h"
#include "ui/TrendPreviewWidget.h"
#include "ui/ResultCard.h"
#include "data/CsvKLineReader.h"
#include "model/KLinePeriod.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
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

    static QWidget *createClosableChip(const QString &text)
    {
        auto *chip = new QWidget();
        chip->setObjectName("StockChip");
        chip->setAttribute(Qt::WA_StyledBackground, true);
        chip->setMinimumHeight(38);
        chip->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

        auto *layout = new QHBoxLayout(chip);
        layout->setContentsMargins(14, 0, 8, 0);
        layout->setSpacing(6);

        auto *label = new QLabel(text);
        label->setObjectName("StockChipLabel");

        auto *closeBtn = new QToolButton();
        closeBtn->setObjectName("StockChipClose");
        closeBtn->setText(QStringLiteral("×"));
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setAutoRaise(true);
        closeBtn->setFixedSize(16, 16);

        QObject::connect(closeBtn, &QToolButton::clicked, chip, [chip]()
                         { chip->deleteLater(); });

        layout->addWidget(label);
        layout->addWidget(closeBtn);

        return chip;
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

    const QVector<KLineBar> bars = CsvKLineReader::readFromFile("D:/wdw/aiselectstock/data/sh000001_daily.csv");
    if (m_chart && !bars.isEmpty()) {
        m_chart->setDailyBars(bars);
        m_chart->setPeriod(KLinePeriod::Daily);
    }
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

    auto *searchEdit = new QLineEdit();
    searchEdit->setObjectName("SearchEdit");
    searchEdit->setPlaceholderText(QStringLiteral("输入股票名称 / 代码 "));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *searchBtn = new QPushButton(QStringLiteral("搜索"));
    searchBtn->setObjectName("SearchButton");
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setFixedWidth(110);
    searchBtn->setMinimumHeight(44);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(searchBtn);

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

    layout->addStretch(1);
    layout->addWidget(searchWrap);
    layout->addStretch(1);
    layout->addWidget(m_minBtn);
    layout->addWidget(m_maxBtn);
    layout->addWidget(m_closeBtn);

    return topBar;
}

QWidget* MainWindow::createStockTagsRow()
{
    auto *row = new QWidget();
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    layout->addWidget(createClosableChip(QStringLiteral("上证指数")));
    layout->addStretch();

    return row;
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

    auto *btnDay = createTimeButton(QStringLiteral("日K"),true);
    auto *btnWeek = createTimeButton(QStringLiteral("周K"));
    auto *btnMonth = createTimeButton(QStringLiteral("月K"));

    timeGroup->addButton(btnDay);
    timeGroup->addButton(btnWeek);
    timeGroup->addButton(btnMonth);

    timeLayout->addWidget(btnDay);
    timeLayout->addWidget(btnWeek);
    timeLayout->addWidget(btnMonth);
    timeLayout->addSpacing(10);
    timeLayout->addStretch();

    connect(btnDay, &QPushButton::clicked, this, [this]() {
        if (m_chart) {
            m_chart->setPeriod(KLinePeriod::Daily);
        }
    });

    connect(btnWeek, &QPushButton::clicked, this, [this]() {
        if (m_chart) {
            m_chart->setPeriod(KLinePeriod::Weekly);
        }
    });

    connect(btnMonth, &QPushButton::clicked, this, [this]() {
        if (m_chart) {
            m_chart->setPeriod(KLinePeriod::Monthly);
        }
    });

    m_chart = new TrendPreviewWidget();

    auto *footerTip = new QLabel(QStringLiteral("说明：本软件仅用于走势筛选与可视化，不提供买卖操作。"));
    footerTip->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");

    layout->addWidget(timeRow);
    layout->addWidget(m_chart, 1);
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
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(12);

    containerLayout->addWidget(new ResultCard(QStringLiteral("中际旭创"),
                                              "300308",
                                              "92"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("比亚迪"),
                                              "002594",
                                              "88"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("沪电股份"),
                                              "002463",
                                              "85"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("寒武纪"),
                                              "688256",
                                              "83"));

    containerLayout->addStretch();

    scrollArea->setWidget(container);

    auto *bottomInfo = new QLabel(QStringLiteral("当前示例共匹配 12 只股票"));
    bottomInfo->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");

    layout->addLayout(headerRow);
    layout->addWidget(subTitle);
    layout->addWidget(scrollArea, 1);
    layout->addWidget(bottomInfo);

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
            border: 1px solid #c98b94;
            border-radius: 16px;
        }

        QWidget#StockChip:hover {
            border: 1px solid #e9aab3;
            background: #fff7f8;
        }

        QLabel#StockChipLabel {
            color: #5e646f;
            font: 600 13px "Microsoft YaHei";
            background: transparent;
        }

        QToolButton#StockChipClose {
            background: transparent;
            border: none;
            color: #a86a72;
            font: 700 12px "Microsoft YaHei";
            padding: 0;
        }

        QToolButton#StockChipClose:hover {
            color: #c71f2f;
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