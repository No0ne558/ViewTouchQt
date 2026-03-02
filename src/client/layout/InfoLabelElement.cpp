// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/InfoLabelElement.cpp

#include "InfoLabelElement.h"
#include "Version.h"

#include <QFont>
#include <QPainter>

namespace vt {

InfoLabelElement::InfoLabelElement(const QString &id,
                                   qreal x, qreal y, qreal w, qreal h,
                                   QGraphicsItem *parent)
    : UiElement(id, ElementType::InfoLabel, x, y, w, h, parent)
{
    m_bgColor   = QColor(40, 40, 45);
    m_textColor = QColor(180, 180, 180);
    m_fontSize  = 18;
    m_cornerRadius = 8.0;
    setAcceptedMouseButtons(Qt::NoButton);  // not interactive

    refreshVersionText();
}

void InfoLabelElement::refreshVersionText()
{
    m_label = QStringLiteral("ViewTouchQt  v%1  ·  %2  ·  %3")
                  .arg(QStringLiteral(VT_VERSION_STRING),
                       QStringLiteral(VT_GIT_HASH),
                       QStringLiteral(VT_BUILD_DATE));
    update();
}

void InfoLabelElement::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem * /*option*/,
                             QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Always draw the background
    painter->setBrush(m_bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(m_rect.adjusted(1, 1, -1, -1),
                             m_cornerRadius, m_cornerRadius);

    painter->setFont(buildFont());
    painter->setPen(m_textColor);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);
}

} // namespace vt
