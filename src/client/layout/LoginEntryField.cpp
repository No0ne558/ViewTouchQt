// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LoginEntryField.cpp

#include "LoginEntryField.h"

#include <QPainter>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>

namespace vt {

LoginEntryField::LoginEntryField(const QString &id,
                                 qreal x, qreal y, qreal w, qreal h,
                                 QGraphicsItem *parent)
    : UiElement(id, ElementType::LoginEntry, x, y, w, h, parent)
{
    // Allow keyboard focus so scene()->focusItem() can be used by PageWidget
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    // Visual defaults
    m_fontSize = 28;
    setBgColor(QColor(245, 245, 245));
    setTextColor(Qt::black);
}

void LoginEntryField::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem * /*option*/,
                           QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor());
    painter->drawRoundedRect(m_rect, cornerRadius(), cornerRadius());

    // Draw masked text left-aligned with some padding
    painter->setFont(buildFont());
    painter->setPen(textColor());

    QString masked;
    masked.fill(m_maskChar, m_value.length());

    const QRectF textRect = m_rect.adjusted(12, 0, -12, 0);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, masked);

    // Optional focus caret
    if (hasFocus()) {
        QFontMetrics fm(buildFont());
        int textWidth = fm.horizontalAdvance(masked);
        qreal cx = textRect.left() + textWidth + 2;
        painter->setPen(QPen(textColor()));
        painter->drawLine(QPointF(cx, textRect.top() + 6), QPointF(cx, textRect.bottom() - 6));
    }
}

void LoginEntryField::appendKey(const QString &k)
{
    if (k.isEmpty())
        return;
    // accept only single characters (digits) for login field
    QChar c = k.at(0);
    if (!c.isDigit())
        return;
    if (m_value.length() >= m_maxLength)
        return;
    m_value.append(c);
    emit valueChanged(m_value);
    update();
}

void LoginEntryField::backspace()
{
    if (m_value.isEmpty())
        return;
    m_value.chop(1);
    emit valueChanged(m_value);
    update();
}

void LoginEntryField::clear()
{
    if (m_value.isEmpty())
        return;
    m_value.clear();
    emit valueChanged(m_value);
    update();
}

void LoginEntryField::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    setFocus();
    update();
}

void LoginEntryField::keyPressEvent(QKeyEvent *event)
{
    if (!event)
        return;

    if (event->key() == Qt::Key_Backspace) {
        backspace();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit submitted(m_value);
        event->accept();
        return;
    }

    const QString txt = event->text();
    if (!txt.isEmpty() && txt.at(0).isDigit()) {
        appendKey(txt);
        event->accept();
        return;
    }

    // not handled
    event->ignore();
}

void LoginEntryField::submit()
{
    emit submitted(m_value);
}

} // namespace vt
