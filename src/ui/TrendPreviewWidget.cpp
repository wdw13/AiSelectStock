#include "ui/TrendPreviewWidget.h"
#include "core/KLineAggregator.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QVector>
#include <cmath>

namespace
{

    static QString formatXAxisMajorDate(const QDate &date, KLinePeriod period, bool forceFull, bool crossYear)
    {
        if (!date.isValid())
        {
            return QString();
        }

        if (period == KLinePeriod::Monthly)
        {
            return (forceFull || crossYear)
                       ? date.toString("yyyy-MM")
                       : date.toString("yyyy-MM");
        }

        if (period == KLinePeriod::Weekly)
        {
            return (forceFull || crossYear)
                       ? date.toString("yyyy-MM-dd")
                       : date.toString("MM-dd");
        }

        // Daily
        return (forceFull || crossYear)
                   ? date.toString("yyyy-MM-dd")
                   : date.toString("MM-dd");
    }

}

TrendPreviewWidget::TrendPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(0);
    setMinimumHeight(420);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void TrendPreviewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    const QVector<KLineBar>& data = currentBars();

    if (data.isEmpty()) {
        return;
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

    const int hLines = 5;
    const int vLines = 6;

    clampRightIndex();

    int firstVisible = qMax(0, m_rightIndex - m_visibleBars + 1);
    int lastVisible = qMin(m_rightIndex, data.size() - 1);

    if (lastVisible < firstVisible)
    {
        return;
    }

    const int visibleCount = lastVisible - firstVisible + 1;
    const double step = plot.width() / visibleCount;
    const double bodyWidth = qBound(3.0, step * 0.62, 28.0);

    auto xAtSlot = [&](int slot) -> double
    {
        return plot.left() + m_panOffsetPx + step * (slot + 0.5);
    };

    double minPrice = data[firstVisible].low;
    double maxPrice = data[firstVisible].high;
    for (int i = firstVisible; i <= lastVisible; ++i) {
        minPrice = qMin(minPrice, data[i].low);
        maxPrice = qMax(maxPrice, data[i].high);
    }

    double pricePadding = (maxPrice - minPrice) * 0.08;
    if (pricePadding < 0.01) {
        pricePadding = 0.01;
    }
    minPrice -= pricePadding;
    maxPrice += pricePadding;

    auto toY = [&](double price) -> double {
        if (qFuzzyCompare(maxPrice, minPrice)) {
            return plot.center().y();
        }
        const double rate = (price - minPrice) / (maxPrice - minPrice);
        return plot.bottom() - rate * plot.height();
    };

    // 网格
    p.setPen(QPen(QColor("#f0d9dd"), 1, Qt::DashLine));
    for (int i = 0; i <= hLines; ++i) {
        const double y = plot.top() + i * (plot.height() / hLines);
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    for (int i = 0; i <= vLines; ++i) {
        const double x = plot.left() + i * (plot.width() / vLines);
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
    }

    // 左侧价格刻度
    p.setPen(QColor("#8a8f99"));
    QFont axisFont("Microsoft YaHei", 8);
    p.setFont(axisFont);

    for (int i = 0; i <= hLines; ++i) {
        const double price = maxPrice - i * (maxPrice - minPrice) / hLines;
        const double y = plot.top() + i * (plot.height() / hLines);
        p.drawText(QRectF(plot.left() - 48, y - 10, 42, 20),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(price, 'f', 2));
    }

    // 裁剪绘图区，防止画到坐标轴外面
    p.save();
    p.setClipRect(plot.adjusted(1, 1, -1, -1));

    auto buildMAPoints = [&](int period) -> QVector<QPointF>
    {
        QVector<QPointF> pts;
        pts.reserve(qMax(0, lastVisible - firstVisible + 1));

        for (int i = firstVisible; i <= lastVisible; ++i)
        {
            if (i < period - 1)
            {
                continue;
            }

            double sum = 0.0;
            for (int j = i - period + 1; j <= i; ++j)
            {
                sum += data[j].close;
            }

            const double ma = sum / period;
            const int slot = i - firstVisible;
            const double x = xAtSlot(slot);
            const double y = toY(ma);

            pts.push_back(QPointF(x, y));
        }

        return pts;
    };

    auto drawMAPath = [&](const QVector<QPointF> &pts, const QColor &color)
    {
        if (pts.size() < 2)
        {
            return;
        }

        QPainterPath path(pts.first());
        for (int i = 1; i < pts.size(); ++i)
        {
            path.lineTo(pts[i]);
        }

        p.setPen(QPen(color, 0.8));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    };

    for (int i = firstVisible; i <= lastVisible; ++i)
    {
        const auto &c = data[i];
        const int slot = i - firstVisible;
        const double xCenter = xAtSlot(slot);

        const double yOpen = toY(c.open);
        const double yClose = toY(c.close);
        const double yHigh = toY(c.high);
        const double yLow = toY(c.low);

        const bool rise = c.close >= c.open;
        const QColor candleColor = rise ? QColor("#d62839") : QColor("#17a673");

        p.setPen(QPen(candleColor, 1.0));
        p.drawLine(QPointF(xCenter, yHigh), QPointF(xCenter, yLow));

        QRectF bodyRect(xCenter - bodyWidth / 2.0,
                        qMin(yOpen, yClose),
                        bodyWidth,
                        qMax(2.0, qAbs(yClose - yOpen)));

        p.setPen(Qt::NoPen);
        p.setBrush(candleColor);
        p.drawRoundedRect(bodyRect, 1.5, 1.5);
    }

    const QVector<QPointF> ma5 = buildMAPoints(5);
    const QVector<QPointF> ma10 = buildMAPoints(10);
    const QVector<QPointF> ma20 = buildMAPoints(20);
    const QVector<QPointF> ma30 = buildMAPoints(30);

    drawMAPath(ma5, QColor("#8f96a3"));  // 灰
    drawMAPath(ma10, QColor("#8e63d2")); // 紫
    drawMAPath(ma20, QColor("#d8b11e")); // 黄
    drawMAPath(ma30, QColor("#4a78d3")); // 蓝

    p.restore();

    // 底部时间标识
    p.setPen(QColor("#8a8f99"));
    QFont bottomFont("Microsoft YaHei", 8);
    p.setFont(bottomFont);

    // ===== 小刻度 =====
    const bool showMinorTicks = step >= 6.0;
    if (showMinorTicks)
    {
        p.setPen(QPen(QColor("#c7cbd3"), 1));
        for (int i = firstVisible; i <= lastVisible; ++i)
        {
            const int slot = i - firstVisible;
            const double x = xAtSlot(slot);

            if (x < plot.left() || x > plot.right())
            {
                continue;
            }

            p.drawLine(QPointF(x, plot.bottom() + 2),
                       QPointF(x, plot.bottom() + 7));
        }
    }

    // ===== 大刻度 =====
    p.setPen(QPen(QColor("#9aa0aa"), 1.2));

    int majorStep = qMax(1, visibleCount / 4);

    int firstMajor = (firstVisible / majorStep) * majorStep;
    if (firstMajor < firstVisible)
    {
        firstMajor += majorStep;
    }

    QFontMetrics fm(bottomFont);

    for (int idx = firstMajor; idx <= lastVisible; idx += majorStep)
    {
        const int slot = idx - firstVisible;
        const double x = xAtSlot(slot);

        if (x < plot.left() || x > plot.right())
        {
            continue;
        }

        p.drawLine(QPointF(x, plot.bottom() + 1),
                   QPointF(x, plot.bottom() + 11));

        const bool forceFull = (idx == firstVisible);
        const bool crossYear = (data[idx].date.year() != data[firstVisible].date.year());

        QString text = formatXAxisMajorDate(
            data[idx].date,
            m_period,
            forceFull,
            crossYear);

        const int textW = qMax(46, fm.horizontalAdvance(text) + 8);

        // 关键：不要边界夹紧，永远以刻度中心对齐
        QRectF textRect(x - textW / 2.0,
                        plot.bottom() + 12,
                        textW,
                        18);

        p.drawText(textRect, Qt::AlignCenter, text);
    }

    // 水印
    QFont wmFont("Microsoft YaHei", 24, QFont::Bold);
    p.setFont(wmFont);
    p.setPen(QColor(214, 40, 57, 22));
    p.drawText(plot, Qt::AlignCenter, QStringLiteral("AISELECTSTOCK"));
}

QRectF TrendPreviewWidget::chartRect() const
{
    QRectF cardRect = rect().adjusted(8, 8, -8, -8);
    return cardRect.adjusted(58, 88, -28, -48);
}

void TrendPreviewWidget::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF pos = event->position();
#else
    const QPointF pos = event->pos();
#endif

    const QRectF plot = chartRect();
    if (!plot.contains(pos)) {
        QWidget::wheelEvent(event);
        return;
    }

    const QVector<KLineBar>& data = currentBars();
    if (data.isEmpty()) {
        return;
    }

    const int oldVisibleBars = m_visibleBars;

    if (event->angleDelta().y() > 0) {
        // 放大：显示更少根
        m_visibleBars = qMax(10, int(std::round(m_visibleBars * 0.85)));
    } else {
        // 缩小：显示更多根
        m_visibleBars = qMin(data.size(), int(std::round(m_visibleBars * 1.15)));
    }

    if (m_visibleBars < 20) m_visibleBars = 20;
    if (m_visibleBars > data.size()) m_visibleBars = data.size();

    // 以鼠标所在位置为中心缩放
    const int oldFirst = qMax(0, m_rightIndex - oldVisibleBars + 1);
    const double ratio = (pos.x() - plot.left()) / plot.width();
    const int anchorIndex = qBound(oldFirst,
                                   oldFirst + int(ratio * oldVisibleBars),
                                   m_rightIndex);

    m_rightIndex = anchorIndex + int((1.0 - ratio) * m_visibleBars);
    clampRightIndex();

    update();
    event->accept();

    m_panOffsetPx = 0.0;
}

void TrendPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const QRectF plot = chartRect();
        if (plot.contains(event->pos()))
        {
            m_panning = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

void TrendPreviewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning)
    {
        const QVector<KLineBar>& data = currentBars();
        if (data.isEmpty()) {
            return;
        }

        const int dx = event->pos().x() - m_lastMousePos.x();
        m_lastMousePos = event->pos();

        const QRectF plot = chartRect();
        if (plot.width() > 0)
        {
            const int firstVisible = qMax(0, m_rightIndex - m_visibleBars + 1);
            const int lastVisible  = qMin(m_rightIndex, data.size() - 1);
            const int visibleCount = lastVisible - firstVisible + 1;

            if (visibleCount > 0)
            {
                const double step = plot.width() / visibleCount;

                // 先累计像素位移
                m_panOffsetPx += dx;

                // 像素残余达到一根柱宽时，再真正移动索引
                while (m_panOffsetPx >= step)
                {
                    m_rightIndex -= 1;
                    m_panOffsetPx -= step;
                }

                while (m_panOffsetPx <= -step)
                {
                    m_rightIndex += 1;
                    m_panOffsetPx += step;
                }

                clampRightIndex();
                update();
            }
        }

        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void TrendPreviewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        unsetCursor();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void TrendPreviewWidget::setDailyBars(const QVector<KLineBar>& bars)
{
    m_dailyBars = bars;
    m_weeklyBars = KLineAggregator::toWeekly(m_dailyBars);
    m_monthlyBars = KLineAggregator::toMonthly(m_dailyBars);

    m_period = KLinePeriod::Daily;

    m_visibleBars = 80;
    m_rightIndex = m_dailyBars.isEmpty() ? -1 : (m_dailyBars.size() - 1);

    update();
}

void TrendPreviewWidget::setPeriod(KLinePeriod period)
{
    if (m_period == period) {
        return;
    }

    m_period = period;

    const QVector<KLineBar>& bars = currentBars();
    m_visibleBars = qMin(80, qMax(20, bars.size()));
    m_rightIndex = bars.isEmpty() ? -1 : (bars.size() - 1);

    update();
}

const QVector<KLineBar>& TrendPreviewWidget::currentBars() const
{
    switch (m_period) {
    case KLinePeriod::Weekly:
        return m_weeklyBars;
    case KLinePeriod::Monthly:
        return m_monthlyBars;
    case KLinePeriod::Daily:
    default:
        return m_dailyBars;
    }
}

void TrendPreviewWidget::clampRightIndex()
{
    const QVector<KLineBar>& bars = currentBars();
    if (bars.isEmpty()) {
        m_rightIndex = -1;
        return;
    }

    if (m_visibleBars < 20) {
        m_visibleBars = 20;
    }
    if (m_visibleBars > bars.size()) {
        m_visibleBars = bars.size();
    }

    if (m_rightIndex < m_visibleBars - 1) {
        m_rightIndex = m_visibleBars - 1;
    }
    if (m_rightIndex >= bars.size()) {
        m_rightIndex = bars.size() - 1;
    }
}
