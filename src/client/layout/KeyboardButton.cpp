// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/KeyboardButton.cpp

#include "KeyboardButton.h"

#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace vt {

KeyboardButton::KeyboardButton(const QString &id,
                               qreal x, qreal y, qreal w, qreal h,
                               const QString &assignedKey,
                               QGraphicsItem *parent)
    : ButtonElement(id, x, y, w, h, ElementType::KeyboardButton, parent)
    , m_assignedKey(assignedKey)
{
    // Label the button with the assigned key by default
    setLabel(m_assignedKey);
    // Slightly smaller font for keypad
    m_fontSize = 28;
}

void KeyboardButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Emit virtual key event for local input handling first
    qDebug() << "[KeyboardButton]" << elementId() << "pressed ->" << m_assignedKey;
    emit virtualKeyPressed(m_assignedKey);
    // Preserve visual click behaviour from ButtonElement
    ButtonElement::mousePressEvent(event);
}

void KeyboardButton::setAssignedKey(const QString &k)
{
    qDebug() << "[KeyboardButton]" << elementId() << "setAssignedKey:" << k;
    m_assignedKey = k;
    setLabel(k);
}

} // namespace vt
