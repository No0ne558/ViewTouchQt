// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LabelElement.cpp

#include "LabelElement.h"

#include <QFont>
#include <QPainter>

namespace vt {

LabelElement::LabelElement(const QString &id,
                           qreal x, qreal y, qreal w, qreal h,
                           QGraphicsItem *parent)
    : UiElement(id, ElementType::Label, x, y, w, h, parent)
{
    m_bgColor   = Qt::transparent;
    m_textColor = Qt::white;
    m_fontSize  = 28;
    setAcceptedMouseButtons(Qt::NoButton);  // labels are not interactive
}

void LabelElement::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem * /*option*/,
                         QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    if (m_drawBg && m_bgColor != Qt::transparent) {
        painter->setBrush(m_bgColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);
    }

    QFont font(QStringLiteral("Sans"), m_fontSize, QFont::Bold);
    painter->setFont(font);
    painter->setPen(m_textColor);
    painter->drawText(m_rect, static_cast<int>(m_alignment) | Qt::AlignVCenter,
                      m_label);
}

} // namespace vt
