// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/ButtonItem.cpp

#include "ButtonItem.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace vt {

ButtonItem::ButtonItem(qreal x, qreal y, qreal w, qreal h,
                       QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_rect(0, 0, w, h)
    , m_currentColor(kColorIdle)
{
    setPos(x, y);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(kFlashDurationMs);
    connect(&m_flashTimer, &QTimer::timeout, this, &ButtonItem::resetColor);
}

QRectF ButtonItem::boundingRect() const
{
    return m_rect;
}

void ButtonItem::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem * /*option*/,
                       QWidget * /*widget*/)
{
    // Rounded rectangle with the current colour.
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_currentColor);
    painter->setPen(QPen(Qt::black, 3));
    painter->drawRect(m_rect);

    // Label
    QFont font("Sans", 32, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::black);
    painter->drawText(m_rect, Qt::AlignCenter, QStringLiteral("PRESS"));
}

void ButtonItem::flashAck()
{
    m_currentColor = kColorActive;
    update();
    m_flashTimer.start();
}

void ButtonItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    // Immediate visual feedback.
    m_currentColor = kColorActive;
    update();
    m_flashTimer.start();

    emit pressed();
}

void ButtonItem::resetColor()
{
    m_currentColor = kColorIdle;
    update();
}

} // namespace vt
