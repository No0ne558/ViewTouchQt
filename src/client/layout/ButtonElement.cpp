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

    m_doubleTapTimer.setSingleShot(true);
    m_doubleTapTimer.setInterval(kDoubleTapMs);
    connect(&m_doubleTapTimer, &QTimer::timeout, this, &ButtonElement::onDoubleTapTimeout);
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
    // Do not perform ack flash for elements configured with 'None'
    if (behavior() == UiElement::ButtonBehavior::None)
        return;

    // Show active colour briefly; resetColor() will restore the
    // element's logical display state (respecting Toggle).
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();
}

void ButtonElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    // Branch behaviour based on element setting
    switch (behavior()) {
    case ButtonBehavior::Blink:
        m_currentColor = m_activeColor;
        update();
        m_flashTimer.start();
        emit clicked(m_id);
        break;
    case ButtonBehavior::Toggle:
        m_toggled = !m_toggled;
        // Cancel any pending flash reset so toggled-on state persists.
        m_flashTimer.stop();
        m_currentColor = m_toggled ? m_activeColor : m_bgColor;
        update();
        emit clicked(m_id);
        break;
    case ButtonBehavior::None:
        emit clicked(m_id);
        break;
    case ButtonBehavior::DoubleTap:
        if (m_pendingTap) {
            // Second tap within timeout — trigger action
            m_pendingTap = false;
            m_doubleTapTimer.stop();
            m_currentColor = m_activeColor;
            update();
            m_flashTimer.start();
            emit clicked(m_id);
        } else {
            // First tap — wait for a possible second tap
            m_pendingTap = true;
            m_doubleTapTimer.start();
        }
        break;
    }
}

void ButtonElement::resetColor()
{
    // Restore to either toggled active colour (for Toggle) or idle bg.
    if (behavior() == UiElement::ButtonBehavior::Toggle && m_toggled)
        m_currentColor = m_activeColor;
    else
        m_currentColor = m_bgColor;
    update();
}

void ButtonElement::onDoubleTapTimeout()
{
    // Double-tap window expired — clear pending state and do nothing.
    m_pendingTap = false;
}

} // namespace vt
