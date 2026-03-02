// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/KeypadButtonElement.cpp

#include "KeypadButtonElement.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

namespace vt {

KeypadButtonElement::KeypadButtonElement(const QString &id,
                                         qreal x, qreal y, qreal w, qreal h,
                                         QGraphicsItem *parent)
    : UiElement(id, ElementType::KeypadButton, x, y, w, h, parent)
    , m_currentColor(m_bgColor)
{
    m_bgColor      = QColor(55, 55, 55);
    m_currentColor = m_bgColor;
    m_textColor    = Qt::white;
    m_cornerRadius = 8.0;
    m_fontSize     = 28;

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(kFlashDurationMs);
    connect(&m_flashTimer, &QTimer::timeout, this, &KeypadButtonElement::resetColor);
}

void KeypadButtonElement::setBgColor(const QColor &c)
{
    UiElement::setBgColor(c);
    if (!m_flashTimer.isActive()) {
        m_currentColor = c;
        update();
    }
}

void KeypadButtonElement::paint(QPainter *painter,
                                const QStyleOptionGraphicsItem * /*option*/,
                                QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_currentColor);
    painter->setPen(QPen(QColor(80, 80, 80), 1));
    painter->drawRoundedRect(m_rect.adjusted(1, 1, -1, -1), m_cornerRadius, m_cornerRadius);

    QFont font(QStringLiteral("Sans"), m_fontSize, QFont::Bold);
    painter->setFont(font);
    painter->setPen(m_textColor);
    painter->drawText(m_rect, Qt::AlignCenter, m_label);
}

void KeypadButtonElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();

    // Emit the key value; if none set, use the label text
    QString val = m_keyValue.isEmpty() ? m_label : m_keyValue;
    emit keyPressed(val);
}

void KeypadButtonElement::resetColor()
{
    m_currentColor = m_bgColor;
    update();
}

} // namespace vt
