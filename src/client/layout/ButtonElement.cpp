// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ButtonElement.cpp

#include "ButtonElement.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace vt {

ButtonElement::ButtonElement(const QString &id,
                             qreal x, qreal y, qreal w, qreal h,
                             ElementType type,
                             QGraphicsItem *parent)
    : UiElement(id, type, x, y, w, h, parent)
    , m_currentColor(m_bgColor)
{
    m_fontSize = 32;

    m_flashTimer.setSingleShot(true);
    m_flashTimer.setInterval(kFlashDurationMs);
    connect(&m_flashTimer, &QTimer::timeout, this, &ButtonElement::resetColor);

    m_doubleTapTimer.setSingleShot(true);
    m_doubleTapTimer.setInterval(kDoubleTapMs);
    connect(&m_doubleTapTimer, &QTimer::timeout, this, &ButtonElement::onDoubleTapTimeout);
}

void ButtonElement::setBgColor(const QColor &c)
{
    UiElement::setBgColor(c);
    // Keep display colour in sync unless mid-flash.
    if (!m_flashTimer.isActive()) {
        m_currentColor = c;
        update();
    }
}

void ButtonElement::setImagePath(const QString &path)
{
    if (m_imagePath == path)
        return;
    m_imagePath = path;
    m_pixmap = QPixmap();
    m_scaledPixmap = QPixmap();
    m_lastScaledSize = QSize();

    if (m_imagePath.isEmpty()) {
        update();
        return;
    }

    QString resolved = m_imagePath;
    QFileInfo fi(resolved);
    if (!fi.isAbsolute()) {
        if (!QFile::exists(resolved)) {
            QByteArray env = qgetenv("VIEWTOUCH_IMG_DIR");
            QString sysDir = env.isEmpty() ? QStringLiteral("/opt/viewtouch/img") : QString::fromUtf8(env);
            QString candidate = QDir(sysDir).filePath(resolved);
            if (QFile::exists(candidate))
                resolved = candidate;
        }
    }

    if (!m_pixmap.load(resolved)) {
        qWarning() << "[ButtonElement] Failed to load image:" << m_imagePath << "resolved:" << resolved;
        m_pixmap = QPixmap();
    }
    update();
}

// Image-only mode accessors are inline in the header.

void ButtonElement::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem * /*option*/,
                          QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    const auto style = edgeStyle();
    // Compute a shadow intensity multiplier from element setting
    double shadowMul = 1.0;
    switch (shadowIntensity()) {
    case UiElement::ShadowIntensity::None:
        shadowMul = 0.0;
        break;
    case UiElement::ShadowIntensity::Min:
        shadowMul = 0.6;
        break;
    case UiElement::ShadowIntensity::Med:
        shadowMul = 1.0;
        break;
    case UiElement::ShadowIntensity::Max:
        shadowMul = 1.5;
        break;
    }

    // Draw base outer shadow behind the element (outside the button)
    if (shadowMul > 0.0 && style != UiElement::EdgeStyle::None) {
        const int baseAlpha = 60;
        const qreal baseOffset = 2.0;
        int a = qBound(1, int(baseAlpha * shadowMul), 255);
        qreal off = baseOffset * shadowMul;
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, a));
        const qreal rr = qMin(m_cornerRadius, qreal(6.0));
        painter->drawRoundedRect(m_rect.translated(off, off), rr, rr);
        painter->restore();
    }
    switch (style) {
    case UiElement::EdgeStyle::Flat:
        painter->setBrush(m_currentColor);
        painter->setPen(QPen(Qt::black, 3));
        painter->drawRect(m_rect);
        break;
    case UiElement::EdgeStyle::Raised: {
        // Original-style framed edge: draw inner shape and outer frame
        const int b = qBound(2, int(qMin(m_rect.width(), m_rect.height()) * 0.06), 8); // frame thickness
        const qreal r = qMin(m_cornerRadius, qreal(6.0));

        // Fill inner area (frame leaves a border)
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor);
        painter->drawRoundedRect(inner, r, r);

        // Frame colours: top/left = light, bottom/right = dark
        QColor light = m_currentColor.lighter(160);
        QColor dark  = m_currentColor.darker(180);

        // Draw frame lines similar to original Layer::Frame behavior
        for (int i = 0; i < b; ++i) {
            // top
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i),
                              QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            // left
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i),
                              QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));

            // bottom
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0),
                              QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            // right
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i),
                              QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }

        // outer shadow handled globally above

        break;
    }
    case UiElement::EdgeStyle::Raised2: {
        const int b = qBound(3, int(qMin(m_rect.width(), m_rect.height()) * 0.07), 10);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor);
        painter->drawRoundedRect(inner, r, r);
        QColor light = m_currentColor.lighter(190);
        QColor dark  = m_currentColor.darker(220);
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        // outer shadow handled globally above
        break;
    }
    case UiElement::EdgeStyle::Raised3: {
        const int b = qBound(4, int(qMin(m_rect.width(), m_rect.height()) * 0.08), 12);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor);
        painter->drawRoundedRect(inner, r, r);
        QColor light = m_currentColor.lighter(220);
        QColor dark  = m_currentColor.darker(240);
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        // outer shadow handled globally above
        break;
    }
    case UiElement::EdgeStyle::Inset: {
        // Original-style inset/frame: swap light/dark on edges
        const int b = qBound(2, int(qMin(m_rect.width(), m_rect.height()) * 0.06), 8);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));

        // Fill inner area
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor.darker(110));
        painter->drawRoundedRect(inner, r, r);

        // Inset flips highlight/shadow
        QColor light = m_currentColor.darker(180); // top/left darker for inset
        QColor dark  = m_currentColor.lighter(160); // bottom/right lighter for inset

        for (int i = 0; i < b; ++i) {
            // top (dark)
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i),
                              QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            // left
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i),
                              QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));

            // bottom (light)
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0),
                              QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            // right
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i),
                              QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }

        break;
    }
    case UiElement::EdgeStyle::Inset2: {
        const int b = qBound(3, int(qMin(m_rect.width(), m_rect.height()) * 0.07), 10);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor.darker(120));
        painter->drawRoundedRect(inner, r, r);
        QColor topcol = m_currentColor.darker(200);
        QColor botcol = m_currentColor.lighter(180);
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(topcol, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(botcol, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        break;
    }
    case UiElement::EdgeStyle::Inset3: {
        const int b = qBound(4, int(qMin(m_rect.width(), m_rect.height()) * 0.08), 12);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor.darker(140));
        painter->drawRoundedRect(inner, r, r);
        QColor topcol = m_currentColor.darker(240);
        QColor botcol = m_currentColor.lighter(200);
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(topcol, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(botcol, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        break;
    }
    case UiElement::EdgeStyle::Double: {
        const int b = qBound(2, int(qMin(m_rect.width(), m_rect.height()) * 0.06), 8);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        // Fill main inner
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor);
        painter->drawRoundedRect(inner, r, r);
        // Outer frame
        QColor light = m_currentColor.lighter(160);
        QColor dark  = m_currentColor.darker(180);
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        // Inner secondary frame
        QRectF inner2 = m_rect.adjusted(b*2, b*2, -b*2, -b*2);
        if (inner2.width() > 0 && inner2.height() > 0) {
            QColor light2 = m_currentColor.lighter(130);
            QColor dark2  = m_currentColor.darker(140);
            for (int i = 0; i < b; ++i) {
                painter->setPen(QPen(light2, 1));
                painter->drawLine(QPointF(inner2.left() + i, inner2.top() + i), QPointF(inner2.right() - i - 1.0, inner2.top() + i));
                painter->drawLine(QPointF(inner2.left() + i, inner2.top() + i), QPointF(inner2.left() + i, inner2.bottom() - i - 1.0));
                painter->setPen(QPen(dark2, 1));
                painter->drawLine(QPointF(inner2.left() + i, inner2.bottom() - i - 1.0), QPointF(inner2.right() - i - 1.0, inner2.bottom() - i - 1.0));
                painter->drawLine(QPointF(inner2.right() - i - 1.0, inner2.top() + i), QPointF(inner2.right() - i - 1.0, inner2.bottom() - i - 1.0));
            }
        }
        break;
    }
    case UiElement::EdgeStyle::Border: {
        const int b = qBound(2, int(qMin(m_rect.width(), m_rect.height()) * 0.06), 8);
        const qreal r = qMin(m_cornerRadius, qreal(6.0));
        QRectF inner = m_rect.adjusted(b, b, -b, -b);
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_currentColor);
        painter->drawRoundedRect(inner, r, r);
        QColor light = m_currentColor.lighter(160);
        QColor dark  = m_currentColor.darker(180);
        // outer frame
        for (int i = 0; i < b; ++i) {
            painter->setPen(QPen(light, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.top() + i));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.top() + i), QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0));
            painter->setPen(QPen(dark, 1));
            painter->drawLine(QPointF(m_rect.left() + i, m_rect.bottom() - i - 1.0), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
            painter->drawLine(QPointF(m_rect.right() - i - 1.0, m_rect.top() + i), QPointF(m_rect.right() - i - 1.0, m_rect.bottom() - i - 1.0));
        }
        // inner inset frame (opposite)
        QRectF inner2 = m_rect.adjusted(b*2, b*2, -b*2, -b*2);
        if (inner2.width() > 0 && inner2.height() > 0) {
            QColor light2 = m_currentColor.darker(180);
            QColor dark2  = m_currentColor.lighter(160);
            for (int i = 0; i < b; ++i) {
                painter->setPen(QPen(light2, 1));
                painter->drawLine(QPointF(inner2.left() + i, inner2.top() + i), QPointF(inner2.right() - i - 1.0, inner2.top() + i));
                painter->drawLine(QPointF(inner2.left() + i, inner2.top() + i), QPointF(inner2.left() + i, inner2.bottom() - i - 1.0));
                painter->setPen(QPen(dark2, 1));
                painter->drawLine(QPointF(inner2.left() + i, inner2.bottom() - i - 1.0), QPointF(inner2.right() - i - 1.0, inner2.bottom() - i - 1.0));
                painter->drawLine(QPointF(inner2.right() - i - 1.0, inner2.top() + i), QPointF(inner2.right() - i - 1.0, inner2.bottom() - i - 1.0));
            }
        }
        break;
    }
    
    case UiElement::EdgeStyle::Outline:
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_currentColor, 3));
        painter->drawRect(m_rect);
        break;
    case UiElement::EdgeStyle::Rounded:
        painter->setBrush(m_currentColor);
        painter->setPen(QPen(Qt::black, 3));
        painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);
        break;
    case UiElement::EdgeStyle::None:
        painter->setBrush(m_currentColor);
        painter->setPen(Qt::NoPen);
        painter->drawRect(m_rect);
        break;
    }

    // If an image is set and loaded, draw it centered and scaled
    if (!m_pixmap.isNull()) {
        QSize targetSize(qMax(1, int(m_rect.width())), qMax(1, int(m_rect.height())));
        if (m_scaledPixmap.isNull() || m_lastScaledSize != targetSize) {
            m_scaledPixmap = m_pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_lastScaledSize = targetSize;
        }
        QPointF topLeft = m_rect.topLeft() + QPointF((m_rect.width() - m_scaledPixmap.width()) / 2.0,
                                                     (m_rect.height() - m_scaledPixmap.height()) / 2.0);
        painter->drawPixmap(topLeft, m_scaledPixmap);

        // If this element is a normal Button (not Image-only) and it has a label,
        // draw the label on top of the image so text overlays the picture.
        if (!m_imageOnly && m_type == ElementType::Button && !m_label.isEmpty()) {
            painter->setFont(buildFont());
            // Draw subtle shadow for readability
            painter->setPen(QColor(0, 0, 0, 160));
            painter->drawText(m_rect.translated(1, 1), Qt::AlignCenter, m_label);
            painter->setPen(m_textColor);
            painter->drawText(m_rect, Qt::AlignCenter, m_label);
        }
    } else {
        painter->setFont(buildFont());
        painter->setPen(m_textColor);
        painter->drawText(m_rect, Qt::AlignCenter, m_label);
    }

        // outer shadow is drawn above (outside the element)
}

void ButtonElement::flashAck()
{
    // Do not perform ack flash for elements configured with 'None'
    if (behavior() == UiElement::ButtonBehavior::None)
        return;

    // Show active colour briefly; resetColor() will restore the
    // element's logical display state (respecting Toggle).
    m_currentColor = m_activeColor;
    update();
    m_flashTimer.start();
}

void ButtonElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    // Branch behaviour based on element setting
    switch (behavior()) {
    case ButtonBehavior::Blink:
        m_currentColor = m_activeColor;
        update();
        m_flashTimer.start();
        emit clicked(m_id);
        break;
    case ButtonBehavior::Toggle:
        m_toggled = !m_toggled;
        // Cancel any pending flash reset so toggled-on state persists.
        m_flashTimer.stop();
        m_currentColor = m_toggled ? m_activeColor : m_bgColor;
        update();
        emit clicked(m_id);
        break;
    case ButtonBehavior::None:
        emit clicked(m_id);
        break;
    case ButtonBehavior::DoubleTap:
        if (m_pendingTap) {
            // Second tap within timeout — trigger action
            m_pendingTap = false;
            m_doubleTapTimer.stop();
            m_currentColor = m_activeColor;
            update();
            m_flashTimer.start();
            emit clicked(m_id);
        } else {
            // First tap — wait for a possible second tap
            m_pendingTap = true;
            m_doubleTapTimer.start();
        }
        break;
    }
}

void ButtonElement::resetColor()
{
    // Restore to either toggled active colour (for Toggle) or idle bg.
    if (behavior() == UiElement::ButtonBehavior::Toggle && m_toggled)
        m_currentColor = m_activeColor;
    else
        m_currentColor = m_bgColor;
    update();
}

void ButtonElement::onDoubleTapTimeout()
{
    // Double-tap window expired — clear pending state and do nothing.
    m_pendingTap = false;
}

} // namespace vt
