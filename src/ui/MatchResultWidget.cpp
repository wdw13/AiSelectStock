#include "ui/MatchResultWidget.h"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

static void addShadow(QWidget *w, int blur = 28, int offsetY = 6)
{
    auto *effect = new QGraphicsDropShadowEffect(w);
    effect->setBlurRadius(blur);
    effect->setOffset(0, offsetY);
    effect->setColor(QColor(120, 15, 25, 35));
    w->setGraphicsEffect(effect);
}

MatchResultWidget::MatchResultWidget(const QString &name,
                       const QString &code,
                       const QString &score,
                       QWidget *parent)
    : QFrame(parent)
{
    setObjectName("MatchResultWidget");
    setFixedHeight(88);
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
    // layout->addWidget(ruleLabel);
    layout->addStretch();
    layout->addLayout(bottomRow);

    connect(viewBtn, &QPushButton::clicked, this, [this, code, name]()
            { emit viewRequested(code, name); });

    addShadow(this, 18, 4);
}