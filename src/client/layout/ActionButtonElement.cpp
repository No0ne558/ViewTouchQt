// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ActionButtonElement.cpp

#include "ActionButtonElement.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace vt {

ActionButtonElement::ActionButtonElement(const QString &id,
                                         qreal x, qreal y, qreal w, qreal h,
                                         QGraphicsItem *parent)
    : UiElement(id, ElementType::ActionButton, x, y, w, h, parent)
    , m_currentColor(m_bgColor)
{
    m_bgColor      = QColor(0, 120, 215);
    m_currentColor = m_bgColor;
    m_textColor    = Qt::white;
    m_cornerRadius = 12.0;
    m_fontSize     = 28;

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(kFlashDurationMs);
    connect(&m_flashTimer, &QTimer::timeout, this, &ActionButtonElement::resetColor);
}

void ActionButtonElement::setBgColor(const QColor &c)
{
    UiElement::setBgColor(c);
    if (!m_flashTimer.isActive()) {
        m_currentColor = c;
        update();
    }
}

void ActionButtonElement::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem * /*option*/,
                                QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_currentColor);
    painter->setPen(QPen(Qt::black, 2));
    painter->drawRoundedRect(m_rect.adjusted(1, 1, -1, -1), m_cornerRadius, m_cornerRadius);

    QFont font(QStringLiteral("Sans"), m_fontSize, QFont::Bold);
    painter->setFont(font);
    painter->setPen(m_textColor);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);
}

void ActionButtonElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();
    emit actionTriggered(m_action, m_targetPage);
}

void ActionButtonElement::resetColor()
{
    m_currentColor = m_bgColor;
    update();
}

} // namespace vt
