// SPDX-License-Identifier: GPL-3.0-or-later

#include "GhostItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vt {

GhostItem::GhostItem(const QSizeF &size, const QString &label, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_rect(0, 0, size.width(), size.height()), m_label(label)
{
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    setZValue(1000);
}

void GhostItem::setSize(const QSizeF &size)
{
    prepareGeometryChange();
    m_rect = QRectF(0, 0, size.width(), size.height());
    update();
}

void GhostItem::setLabel(const QString &label)
{
    m_label = label;
    update();
}

QRectF GhostItem::boundingRect() const
{
    return m_rect;
}

void GhostItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor fill(0, 120, 215, 80);   // semi-transparent blue fill
    QColor stroke(0, 120, 215, 180);

    painter->setBrush(fill);
    painter->setPen(QPen(stroke, 1.0));
    painter->drawRoundedRect(m_rect, 6.0, 6.0);

    if (!m_label.isEmpty()) {
        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        painter->setFont(f);
        QRectF textRect = m_rect.adjusted(6, 6, -6, -6);
        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_label);
    }
}

} // namespace vt
