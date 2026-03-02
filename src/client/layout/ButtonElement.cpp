// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ButtonElement.cpp

#include "ButtonElement.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace vt {

ButtonElement::ButtonElement(const QString &id,
                             qreal x, qreal y, qreal w, qreal h,
                             QGraphicsItem *parent)
    : UiElement(id, ElementType::Button, x, y, w, h, parent)
    , m_currentColor(m_bgColor)
{
    m_fontSize = 32;

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(kFlashDurationMs);
    connect(&m_flashTimer, &QTimer::timeout, this, &ButtonElement::resetColor);
}

void ButtonElement::setBgColor(const QColor &c)
{
    UiElement::setBgColor(c);
    // Keep display colour in sync unless mid-flash.
    if (!m_flashTimer.isActive()) {
        m_currentColor = c;
        update();
    }
}

void ButtonElement::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem * /*option*/,
                          QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_currentColor);
    painter->setPen(QPen(Qt::black, 3));
    painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);

    painter->setFont(buildFont());
    painter->setPen(m_textColor);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);
}

void ButtonElement::flashAck()
{
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();
}

void ButtonElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();
    emit clicked(m_id);
}

void ButtonElement::resetColor()
{
    m_currentColor = m_bgColor;
    update();
}

} // namespace vt
