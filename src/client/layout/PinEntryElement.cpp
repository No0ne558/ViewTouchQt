// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PinEntryElement.cpp

#include "PinEntryElement.h"

#include <QFont>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace vt {

PinEntryElement::PinEntryElement(const QString &id,
                                 qreal x, qreal y, qreal w, qreal h,
                                 QGraphicsItem *parent)
    : UiElement(id, ElementType::PinEntry, x, y, w, h, parent)
{
    m_bgColor      = QColor(20, 20, 20);
    m_textColor    = Qt::white;
    m_cornerRadius = 8.0;
    m_fontSize     = 36;
    m_label        = QStringLiteral("Enter PIN");

    setFlag(QGraphicsItem::ItemIsFocusable, true);
}

void PinEntryElement::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem * /*option*/,
                            QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Background
    painter->setBrush(m_bgColor);
    bool focused = hasFocus();
    QPen borderPen(focused ? QColor(0, 120, 215) : QColor(80, 80, 80), focused ? 3 : 2);
    painter->setPen(borderPen);
    painter->drawRoundedRect(m_rect.adjusted(1, 1, -1, -1), m_cornerRadius, m_cornerRadius);

    // Display text
    QString display;
    if (m_pin.isEmpty()) {
        // Placeholder
        painter->setPen(QColor(100, 100, 100));
        display = m_label;
    } else {
        painter->setPen(m_textColor);
        display = m_masked ? QString(m_pin.length(), QChar(0x2022))  // bullet dots
                           : m_pin;
    }

    QFont font = buildFont();
    painter->setFont(font);

    // Add some left padding
    QRectF textRect = m_rect.adjusted(16, 0, -16, 0);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, display);

    // Cursor blink indicator (simple line at the end of text)
    if (focused && !m_pin.isEmpty()) {
        QFontMetrics fm(font);
        qreal textW = fm.horizontalAdvance(display);
        qreal cursorX = 16 + textW + 4;
        if (cursorX < m_rect.width() - 16) {
            painter->setPen(QPen(m_textColor, 2));
            painter->drawLine(QPointF(cursorX, m_rect.height() * 0.2),
                              QPointF(cursorX, m_rect.height() * 0.8));
        }
    }
}

void PinEntryElement::appendChar(const QChar &ch)
{
    if (m_maxLength > 0 && m_pin.length() >= m_maxLength)
        return;

    m_pin.append(ch);
    update();
    emit pinChanged(m_pin);
}

void PinEntryElement::backspace()
{
    if (m_pin.isEmpty())
        return;

    m_pin.chop(1);
    update();
    emit pinChanged(m_pin);
}

void PinEntryElement::clearPin()
{
    if (m_pin.isEmpty())
        return;

    m_pin.clear();
    update();
    emit pinChanged(m_pin);
}

void PinEntryElement::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace) {
        backspace();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit enterPressed();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        clearPin();
        event->accept();
        return;
    }

    // Accept printable characters (digits, letters)
    QString text = event->text();
    if (!text.isEmpty()) {
        QChar ch = text.at(0);
        if (ch.isPrint()) {
            appendChar(ch);
            event->accept();
            return;
        }
    }

    UiElement::keyPressEvent(event);
}

void PinEntryElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Gain focus when clicked (outside of edit mode, the event filter won't consume this)
    setFocus(Qt::MouseFocusReason);
    event->accept();
}

} // namespace vt
