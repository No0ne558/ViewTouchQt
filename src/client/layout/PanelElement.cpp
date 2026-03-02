// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PanelElement.cpp

#include "PanelElement.h"

#include <QPainter>

namespace vt {

PanelElement::PanelElement(const QString &id,
                           qreal x, qreal y, qreal w, qreal h,
                           QGraphicsItem *parent)
    : UiElement(id, ElementType::Panel, x, y, w, h, parent)
{
    m_bgColor = QColor(50, 50, 50);   // dark panel
    setAcceptedMouseButtons(Qt::NoButton);  // panels are not interactive
    setZValue(-1);  // render behind other elements
}

void PanelElement::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem * /*option*/,
                         QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_bgColor);

    if (m_borderColor != Qt::transparent && m_borderWidth > 0) {
        painter->setPen(QPen(m_borderColor, m_borderWidth));
    } else {
        painter->setPen(Qt::NoPen);
    }

    painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);
}

} // namespace vt
