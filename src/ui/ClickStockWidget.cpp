#include "ui/ClickStockWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QToolButton>

ClickableStockChip::ClickableStockChip(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    setObjectName("StockChip");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(38);
    setMinimumWidth(96);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 8, 0);
    layout->setSpacing(6);

    m_label = new QLabel(text, this);
    m_label->setObjectName("StockChipLabel");

    m_closeBtn = new QToolButton(this);
    m_closeBtn->setObjectName("StockChipClose");
    m_closeBtn->setText(QStringLiteral("×"));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setAutoRaise(true);
    m_closeBtn->setFixedSize(16, 16);

    layout->addWidget(m_label);
    layout->addWidget(m_closeBtn);

    connect(m_closeBtn, &QToolButton::clicked, this, [this]() {
        emit closeRequested(this);
        deleteLater();
    });
}

QString ClickableStockChip::text() const
{
    return m_label ? m_label->text() : QString();
}

void ClickableStockChip::setClosable(bool closable)
{
    if (!m_closeBtn) {
        return;
    }

    m_closeBtn->setVisible(closable);
    m_closeBtn->setEnabled(closable);
}

void ClickableStockChip::mousePressEvent(QMouseEvent *event)
{
    if (m_closeBtn && m_closeBtn->geometry().contains(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

    emit clicked();
    QWidget::mousePressEvent(event);
}