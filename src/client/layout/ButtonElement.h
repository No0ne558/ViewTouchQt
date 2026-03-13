// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ButtonElement.h — Touchable button with flash feedback.

#ifndef VT_BUTTON_ELEMENT_H
#define VT_BUTTON_ELEMENT_H

#include "UiElement.h"
#include <QTimer>

namespace vt {

class ButtonElement : public UiElement {
    Q_OBJECT
public:
    explicit ButtonElement(const QString &id,
                           qreal x, qreal y, qreal w, qreal h,
                           QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Play yellow-flash feedback (e.g. on server ack).
    void flashAck();

    /// Colours for idle and active (flash) states.
    void setActiveColor(const QColor &c) { m_activeColor = c; }
    const QColor &activeColor() const { return m_activeColor; }

    /// Override to keep the current display colour in sync.
    void setBgColor(const QColor &c) override;

signals:
    void clicked(const QString &elementId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    void resetColor();

private:
    QColor m_currentColor;
    QColor m_activeColor = QColor(255, 220, 50);  // yellow
    QTimer m_flashTimer;
    static constexpr int kFlashDurationMs = 200;
    // Toggle state for Toggle behaviour
    bool m_toggled = false;

    // Double-tap handling
    QTimer m_doubleTapTimer;
    bool m_pendingTap = false;
    static constexpr int kDoubleTapMs = 300;

private slots:
    void onDoubleTapTimeout();
};

} // namespace vt

#endif // VT_BUTTON_ELEMENT_H
