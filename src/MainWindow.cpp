#include "MainWindow.h"

#include <QApplication>
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
#include <QVBoxLayout>
#include <QVector>
#include <QMouseEvent>

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

    static QPushButton* createChipButton(const QString &text)
    {
        auto *btn = new QPushButton(text);
        btn->setObjectName("StockChip");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(38);
        return btn;
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

// ----------------------------- TrendPreviewWidget -----------------------------

TrendPreviewWidget::TrendPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(760, 520);
}

void TrendPreviewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    struct Candle
    {
        double open;
        double close;
        double high;
        double low;
    };

    const QVector<Candle> data = {
        {13.2, 13.8, 14.0, 13.0},
        {13.8, 13.6, 14.1, 13.5},
        {13.6, 14.3, 14.6, 13.4},
        {14.3, 14.8, 15.0, 14.1},
        {14.8, 14.5, 15.1, 14.4},
        {14.5, 15.2, 15.4, 14.3},
        {15.2, 15.7, 16.0, 15.0},
        {15.7, 15.4, 15.9, 15.2},
        {15.4, 16.1, 16.3, 15.2},
        {16.1, 16.6, 16.9, 15.8},
        {16.6, 16.3, 16.8, 16.0},
        {16.3, 16.9, 17.1, 16.2},
        {16.9, 17.4, 17.6, 16.7},
        {17.4, 17.2, 17.5, 17.0},
        {17.2, 17.9, 18.2, 17.1},
        {17.9, 18.3, 18.5, 17.7},
        {18.3, 18.1, 18.4, 17.9},
        {18.1, 18.8, 19.0, 18.0}
    };

    double minPrice = data[0].low;
    double maxPrice = data[0].high;
    for (const auto &c : data) {
        minPrice = qMin(minPrice, c.low);
        maxPrice = qMax(maxPrice, c.high);
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    QRectF cardRect = rect().adjusted(8, 8, -8, -8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#fff7f8"));
    p.drawRoundedRect(cardRect, 20, 20);

    p.setPen(QPen(QColor("#f1d7db"), 1.2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(cardRect, 20, 20);

    p.setPen(QColor("#4a4d57"));
    QFont titleFont("Microsoft YaHei", 12, QFont::Bold);
    p.setFont(titleFont);
    p.drawText(QRectF(cardRect.left() + 24, cardRect.top() + 18, 260, 30),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("走势预览 / K线匹配示意"));

    p.setPen(QColor("#8d919c"));
    QFont subFont("Microsoft YaHei", 9);
    p.setFont(subFont);
    p.drawText(QRectF(cardRect.left() + 24, cardRect.top() + 48, 450, 22),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("当前展示为界面占位图，后续可替换成真实 K 线绘制或图表组件"));

    QRectF plot = cardRect.adjusted(58, 88, -28, -48);

    // 网格
    p.setPen(QPen(QColor("#f0d9dd"), 1, Qt::DashLine));
    const int hLines = 5;
    const int vLines = 6;

    for (int i = 0; i <= hLines; ++i) {
        const double y = plot.top() + i * (plot.height() / hLines);
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    for (int i = 0; i <= vLines; ++i) {
        const double x = plot.left() + i * (plot.width() / vLines);
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }

    auto toY = [&](double price) -> double {
        if (qFuzzyCompare(maxPrice, minPrice)) {
            return plot.center().y();
        }
        const double rate = (price - minPrice) / (maxPrice - minPrice);
        return plot.bottom() - rate * plot.height();
    };

    // 左侧价格刻度
    p.setPen(QColor("#8a8f99"));
    QFont axisFont("Microsoft YaHei", 8);
    p.setFont(axisFont);

    for (int i = 0; i <= hLines; ++i) {
        const double price = maxPrice - i * (maxPrice - minPrice) / hLines;
        const double y = plot.top() + i * (plot.height() / hLines);
        p.drawText(QRectF(plot.left() - 48, y - 10, 42, 20),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(price, 'f', 1));
    }

    // 蜡烛图
    const double step = plot.width() / data.size();
    const double bodyWidth = step * 0.55;

    QVector<QPointF> linePoints;
    linePoints.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &c = data[i];
        const double xCenter = plot.left() + step * (i + 0.5);
        const double yOpen  = toY(c.open);
        const double yClose = toY(c.close);
        const double yHigh  = toY(c.high);
        const double yLow   = toY(c.low);

        const bool rise = c.close >= c.open;
        const QColor candleColor = rise ? QColor("#d62839") : QColor("#17a673");

        p.setPen(QPen(candleColor, 1.5));
        p.drawLine(QPointF(xCenter, yHigh), QPointF(xCenter, yLow));

        QRectF bodyRect(xCenter - bodyWidth / 2.0,
                        qMin(yOpen, yClose),
                        bodyWidth,
                        qMax(2.0, qAbs(yClose - yOpen)));

        p.setPen(Qt::NoPen);
        p.setBrush(candleColor);
        p.drawRoundedRect(bodyRect, 2, 2);

        linePoints.push_back(QPointF(xCenter, yClose));
    }

    // 趋势线
    if (!linePoints.isEmpty()) {
        QPainterPath trendPath(linePoints.first());
        for (int i = 1; i < linePoints.size(); ++i) {
            trendPath.lineTo(linePoints[i]);
        }

        p.setPen(QPen(QColor("#ff8c42"), 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(trendPath);
    }

    // 底部时间标识
    p.setPen(QColor("#8a8f99"));
    for (int i = 0; i < data.size(); i += 3) {
        const double x = plot.left() + step * (i + 0.5);
        p.drawText(QRectF(x - 18, plot.bottom() + 8, 36, 18),
                   Qt::AlignCenter,
                   QString("T%1").arg(i + 1));
    }

    // 水印
    QFont wmFont("Microsoft YaHei", 24, QFont::Bold);
    p.setFont(wmFont);
    p.setPen(QColor(214, 40, 57, 22));
    p.drawText(plot, Qt::AlignCenter, QStringLiteral("AISELECTSTOCK"));

    // 右上角图例
    p.setFont(QFont("Microsoft YaHei", 8));
    p.setPen(QColor("#d62839"));
    p.drawText(QRectF(plot.right() - 170, plot.top() - 28, 80, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("■ 上涨"));
    p.setPen(QColor("#17a673"));
    p.drawText(QRectF(plot.right() - 100, plot.top() - 28, 80, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("■ 下跌"));
    p.setPen(QColor("#ff8c42"));
    p.drawText(QRectF(plot.right() - 36, plot.top() - 28, 80, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("— 趋势"));
}

// ----------------------------- ResultCard -----------------------------

ResultCard::ResultCard(const QString &name,
                       const QString &code,
                       const QString &rule,
                       const QString &score,
                       QWidget *parent)
    : QFrame(parent)
{
    setObjectName("ResultCard");
    setFixedHeight(122);
    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    auto *nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font: 700 16px 'Microsoft YaHei'; color: #2c2f36;");

    auto *codeLabel = new QLabel(code);
    codeLabel->setAlignment(Qt::AlignCenter);
    codeLabel->setFixedHeight(24);
    codeLabel->setStyleSheet(
        "background:#fff0f2;"
        "color:#c81d25;"
        "border:1px solid #f4c8cd;"
        "border-radius:12px;"
        "padding:2px 10px;"
        "font: 600 11px 'Microsoft YaHei';"
    );

    topRow->addWidget(nameLabel);
    topRow->addStretch();
    topRow->addWidget(codeLabel);

    auto *ruleLabel = new QLabel(rule);
    ruleLabel->setWordWrap(true);
    ruleLabel->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #70757f;");

    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    auto *scoreLabel = new QLabel(QStringLiteral("匹配度 %1").arg(score));
    scoreLabel->setFixedHeight(26);
    scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLabel->setStyleSheet(
        "background:#d62839;"
        "color:white;"
        "border:none;"
        "border-radius:13px;"
        "padding:2px 12px;"
        "font: 600 11px 'Microsoft YaHei';"
    );

    auto *viewBtn = new QPushButton(QStringLiteral("查看"));
    viewBtn->setCursor(Qt::PointingHandCursor);
    viewBtn->setFixedSize(68, 28);
    viewBtn->setStyleSheet(
        "QPushButton {"
        "  background:#ffffff;"
        "  color:#b51622;"
        "  border:1px solid #efc4c9;"
        "  border-radius:14px;"
        "  font: 600 11px 'Microsoft YaHei';"
        "}"
        "QPushButton:hover {"
        "  background:#fff3f4;"
        "}"
    );

    bottomRow->addWidget(scoreLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(viewBtn);

    layout->addLayout(topRow);
    layout->addWidget(ruleLabel);
    layout->addStretch();
    layout->addLayout(bottomRow);

    addShadow(this, 18, 4);
}

// ----------------------------- MainWindow -----------------------------

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setObjectName("MainWindow");
    setWindowTitle("aiselectstock");
    resize(1480, 920);
    setMinimumSize(1280, 780);
    setAttribute(Qt::WA_StyledBackground, true);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);   // 去掉外围留白
    mainLayout->setSpacing(0);

    QWidget *sidebar = createSidebar();
    sidebar->setFixedWidth(150);
    mainLayout->addWidget(sidebar);

    auto *contentArea = new QWidget();
    contentArea->setAttribute(Qt::WA_StyledBackground, true);

    auto *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(18, 18, 18, 18);
    contentLayout->setSpacing(14);

    contentLayout->addWidget(createTopBar());
    contentLayout->addWidget(createStockTagsRow());

    auto *bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(14);
    bodyLayout->addWidget(createChartPanel(), 1);
    bodyLayout->addWidget(createResultPanel());

    contentLayout->addLayout(bodyLayout, 1);

    mainLayout->addWidget(contentArea, 1);

    applyTheme();
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

    auto *aiSelectBtn = createSideButton(QStringLiteral("AI选股"), true);
    auto *rankBtn     = createSideButton(QStringLiteral("榜单"));
    auto *strategyBtn = createSideButton(QStringLiteral("策略"));
    auto *monitorBtn  = createSideButton(QStringLiteral("监控"));

    auto *group = new QButtonGroup(sidebar);
    group->setExclusive(true);
    group->addButton(aiSelectBtn);
    group->addButton(rankBtn);
    group->addButton(strategyBtn);
    group->addButton(monitorBtn);

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
    layout->addWidget(aiSelectBtn);
    layout->addWidget(rankBtn);
    layout->addWidget(strategyBtn);
    layout->addWidget(monitorBtn);
    layout->addStretch();
    layout->addWidget(userCard);

    return sidebar;
}

QWidget* MainWindow::createTopBar()
{
    auto *topBar = new QWidget();
    topBar->setObjectName("TopBar");

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
    searchLayout->setContentsMargins(14, 8, 8, 8);
    searchLayout->setSpacing(8);

    auto *searchEdit = new QLineEdit();
    searchEdit->setObjectName("SearchEdit");
    searchEdit->setPlaceholderText(QStringLiteral("输入股票名称 / 代码 "));
    searchEdit->setClearButtonEnabled(true);

    auto *searchBtn = new QPushButton(QStringLiteral("搜索"));
    searchBtn->setObjectName("SearchButton");
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setFixedWidth(110);

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(searchBtn);

    auto *closeBtn = new QPushButton(QStringLiteral("×"));
    closeBtn->setObjectName("CloseButton");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(56, 56);

    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    layout->addStretch(1);
    layout->addWidget(searchWrap);
    layout->addStretch(1);
    layout->addWidget(closeBtn);

    return topBar;
}

QWidget* MainWindow::createStockTagsRow()
{
    auto *row = new QWidget();
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *label = new QLabel(QStringLiteral("热门示例"));
    label->setStyleSheet("font: 700 13px 'Microsoft YaHei'; color: #6a6f79;");
    label->setFixedWidth(82);

    layout->addWidget(label);
    layout->addWidget(createChipButton(QStringLiteral("贵州茅台")));
    layout->addWidget(createChipButton(QStringLiteral("比亚迪")));
    layout->addWidget(createChipButton(QStringLiteral("中际旭创")));
    layout->addWidget(createChipButton(QStringLiteral("寒武纪")));
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

    auto *btn5d = createTimeButton(QStringLiteral("五日"), true);
    auto *btnDay = createTimeButton(QStringLiteral("日K"));
    auto *btnWeek = createTimeButton(QStringLiteral("周K"));
    auto *btnMonth = createTimeButton(QStringLiteral("月K"));

    timeGroup->addButton(btn5d);
    timeGroup->addButton(btnDay);
    timeGroup->addButton(btnWeek);
    timeGroup->addButton(btnMonth);

    auto *trendHint = new QLabel(QStringLiteral("当前条件：五日抬升 + 日K突破 + 周K共振"));
    trendHint->setStyleSheet("font: 12px 'Microsoft YaHei'; color: #858b95;");

    timeLayout->addWidget(btn5d);
    timeLayout->addWidget(btnDay);
    timeLayout->addWidget(btnWeek);
    timeLayout->addWidget(btnMonth);
    timeLayout->addSpacing(10);
    timeLayout->addWidget(trendHint);
    timeLayout->addStretch();

    auto *chart = new TrendPreviewWidget();

    auto *footerTip = new QLabel(QStringLiteral("说明：本软件仅用于走势筛选与可视化，不提供买卖操作。"));
    footerTip->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");

    layout->addWidget(timeRow);
    layout->addWidget(chart, 1);
    layout->addWidget(footerTip);

    addShadow(panel, 28, 6);
    return panel;
}

QWidget* MainWindow::createResultPanel()
{
    auto *panel = new QWidget();
    panel->setObjectName("ResultPanel");
    panel->setAttribute(Qt::WA_StyledBackground, true);
    panel->setFixedWidth(330);

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

    auto *subTitle = new QLabel(QStringLiteral("根据你的走势条件筛出的候选票（示例数据）"));
    subTitle->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #8b909a;");

    auto *tagRow = new QWidget();
    auto *tagLayout = new QHBoxLayout(tagRow);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->setSpacing(8);

    auto *tag1 = new QLabel(QStringLiteral("放量"));
    auto *tag2 = new QLabel(QStringLiteral("突破"));
    auto *tag3 = new QLabel(QStringLiteral("多头"));
    for (QLabel *tag : {tag1, tag2, tag3}) {
        tag->setAlignment(Qt::AlignCenter);
        tag->setFixedHeight(28);
        tag->setStyleSheet(
            "background:#fff0f2;"
            "color:#c81d25;"
            "border:1px solid #f3c6cc;"
            "border-radius:14px;"
            "padding:0 12px;"
            "font: 600 10px 'Microsoft YaHei';"
        );
        tagLayout->addWidget(tag);
    }
    tagLayout->addStretch();

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
                                              QStringLiteral("五日走强 / 日K放量突破 / 结构完整"),
                                              "92"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("比亚迪"),
                                              "002594",
                                              QStringLiteral("周K抬高 / 月K趋势修复 / 均线粘合后发散"),
                                              "88"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("沪电股份"),
                                              "002463",
                                              QStringLiteral("均线多头 / 回踩确认 / 短期量价配合"),
                                              "85"));

    containerLayout->addWidget(new ResultCard(QStringLiteral("寒武纪"),
                                              "688256",
                                              QStringLiteral("趋势延续 / 强势震荡 / 结构匹配度较高"),
                                              "83"));

    containerLayout->addStretch();

    scrollArea->setWidget(container);

    auto *bottomInfo = new QLabel(QStringLiteral("当前示例共匹配 12 只股票"));
    bottomInfo->setStyleSheet("font: 11px 'Microsoft YaHei'; color: #9aa0aa;");

    layout->addLayout(headerRow);
    layout->addWidget(subTitle);
    layout->addWidget(tagRow);
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
            border-radius: 14px;
            padding: 0 20px;
            font: 700 15px "Microsoft YaHei";
        }

        QPushButton#SearchButton:hover {
            background: #be1f2f;
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

        QPushButton#StockChip {
            background: white;
            color: #5e646f;
            border: 1px solid #efd2d7;
            border-radius: 16px;
            padding: 0 18px;
            font: 600 13px "Microsoft YaHei";
        }

        QPushButton#StockChip:hover {
            color: #c71f2f;
            border: 1px solid #e9aab3;
            background: #fff7f8;
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