#include "ui/TrendPreviewWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QVector>
#include <cmath>

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

    const double scaledWidth = plot.width() * m_xScale;
    const double step = scaledWidth / data.size();
    const double bodyWidth = step * 0.55;

    int firstVisible = static_cast<int>(std::floor((-m_xOffset) / step));
    int lastVisible  = static_cast<int>(std::ceil((plot.width() - m_xOffset) / step));

    if (firstVisible < 0) firstVisible = 0;
    if (lastVisible >= data.size()) lastVisible = data.size() - 1;
    if (lastVisible < firstVisible) {
        firstVisible = 0;
        lastVisible = data.size() - 1;
    }

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

    QVector<QPointF> linePoints;
    linePoints.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        const auto &c = data[i];
        const double xCenter = plot.left() + m_xOffset + step * (i + 0.5);

        if (xCenter < plot.left() - step || xCenter > plot.right() + step) {
            continue;
        }

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

    if (!linePoints.isEmpty()) {
        QPainterPath trendPath(linePoints.first());
        for (int i = 1; i < linePoints.size(); ++i) {
            trendPath.lineTo(linePoints[i]);
        }

        p.setPen(QPen(QColor("#ff8c42"), 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(trendPath);
    }

    p.restore();

    // 底部时间标识
    p.setPen(QColor("#8a8f99"));
    for (int i = 0; i < data.size(); i += 3) {
        const double x = plot.left() + m_xOffset + step * (i + 0.5);
        if (x >= plot.left() && x <= plot.right()) {
            p.drawText(QRectF(x - 18, plot.bottom() + 8, 36, 18),
                       Qt::AlignCenter,
                       QString("T%1").arg(i + 1));
        }
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

void TrendPreviewWidget::clampXOffset(const QRectF &plot)
{
    const double scaledWidth = plot.width() * m_xScale;

    if (scaledWidth <= plot.width()) {
        m_xOffset = 0.0;
        return;
    }

    const double minOffset = plot.width() - scaledWidth; // 负值
    if (m_xOffset < minOffset) {
        m_xOffset = minOffset;
    }
    if (m_xOffset > 0.0) {
        m_xOffset = 0.0;
    }
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

    const double oldScale = m_xScale;
    const double factor = (event->angleDelta().y() > 0) ? 1.15 : (1.0 / 1.15);
    m_xScale *= factor;

    if (m_xScale < 1.0) m_xScale = 1.0;
    if (m_xScale > 8.0) m_xScale = 8.0;

    // 以鼠标所在位置为缩放中心，只影响 X
    const double oldScaledWidth = plot.width() * oldScale;
    const double newScaledWidth = plot.width() * m_xScale;

    double ratio = 0.0;
    if (oldScaledWidth > 0.0) {
        ratio = (pos.x() - plot.left() - m_xOffset) / oldScaledWidth;
    }

    m_xOffset = pos.x() - plot.left() - ratio * newScaledWidth;
    clampXOffset(plot);

    update();
    event->accept();
}

void TrendPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QPointF pos = event->position();
#else
        const QPointF pos = event->pos();
#endif
        if (chartRect().contains(pos)) {
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
    if (m_panning) {
        const int dx = event->pos().x() - m_lastMousePos.x();
        m_lastMousePos = event->pos();

        m_xOffset += dx;
        clampXOffset(chartRect());

        update();
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
