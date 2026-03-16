// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/UiElement.cpp

#include "UiElement.h"

namespace vt {

UiElement::UiElement(const QString &id, ElementType type,
                     qreal x, qreal y, qreal w, qreal h,
                     QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_rect(0, 0, w, h)
    , m_id(id)
    , m_type(type)
{
    setPos(x, y);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    // Ensure initial z-order matches the layer (default 0)
    setZValue(static_cast<qreal>(m_layer));
}

QRectF UiElement::boundingRect() const
{
    return m_rect;
}

void UiElement::moveTo(qreal x, qreal y)
{
    setPos(x, y);
    emit elementMoved(x, y);
}

void UiElement::resizeTo(qreal w, qreal h)
{
    prepareGeometryChange();
    m_rect = QRectF(0, 0, w, h);
    update();
    emit elementResized(w, h);
}

void UiElement::setLabel(const QString &text)
{
    if (m_label != text) {
        m_label = text;
        update();
        emit labelChanged();
    }
}

void UiElement::setBgColor(const QColor &c)
{
    m_bgColor = c;
    update();
}

void UiElement::setTextColor(const QColor &c)
{
    m_textColor = c;
    update();
}

void UiElement::setCornerRadius(qreal r)
{
    m_cornerRadius = r;
    update();
}

void UiElement::setEdgeStyle(UiElement::EdgeStyle s)
{
    if (m_edgeStyle != s) {
        m_edgeStyle = s;
        update();
    }
}

void UiElement::setFontSize(int size)
{
    m_fontSize = size;
    update();
}

void UiElement::setFontFamily(const QString &family)
{
    m_fontFamily = family;
    update();
}

void UiElement::setFontBold(bool bold)
{
    m_fontBold = bold;
    update();
}

QFont UiElement::buildFont() const
{
    QFont f(m_fontFamily, m_fontSize, m_fontBold ? QFont::Bold : QFont::Normal);
    return f;
}

QStringList UiElement::availableFontFamilies()
{
    return {
        QStringLiteral("Sans"),
        QStringLiteral("Noto Sans"),
        QStringLiteral("Open Sans"),
        QStringLiteral("Montserrat"),
        QStringLiteral("Liberation Sans"),
        QStringLiteral("Cantarell"),
        QStringLiteral("DejaVu Sans"),
        QStringLiteral("Droid Sans"),
        QStringLiteral("Comfortaa"),
        QStringLiteral("Carlito"),
        QStringLiteral("Caladea"),
        QStringLiteral("Serif"),
        QStringLiteral("Liberation Serif"),
        QStringLiteral("Nimbus Roman"),
        QStringLiteral("Monospace"),
        QStringLiteral("Liberation Mono"),
        QStringLiteral("Source Code Pro"),
    };
}

} // namespace vt
