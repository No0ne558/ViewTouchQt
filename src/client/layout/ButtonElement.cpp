// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ButtonElement.cpp

#include "ButtonElement.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
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
    painter->setBrush(m_currentColor);
    painter->setPen(QPen(Qt::black, 3));
    painter->drawRoundedRect(m_rect, m_cornerRadius, m_cornerRadius);

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
