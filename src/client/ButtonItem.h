// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/ButtonItem.h — A touchable, colour-flashing button inside QGraphicsScene.

#ifndef VT_BUTTON_ITEM_H
#define VT_BUTTON_ITEM_H

#include <QGraphicsObject>
#include <QTimer>

namespace vt {

/// A rectangular button rendered inside a QGraphicsScene.
/// Grey by default; turns yellow on press, then back to grey after a timeout.
class ButtonItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit ButtonItem(qreal x, qreal y, qreal w, qreal h,
                        QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Call this to play the yellow-flash feedback (e.g. on server ack).
    void flashAck();

signals:
    /// Emitted when the button is clicked / tapped.
    void pressed();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    void resetColor();

private:
    QRectF m_rect;
    QColor m_currentColor;
    QTimer m_flashTimer;

    static constexpr int kFlashDurationMs = 200;

    inline static const QColor kColorIdle    = QColor(160, 160, 160);  // grey
    inline static const QColor kColorActive  = QColor(255, 220,  50);  // yellow
};

} // namespace vt

#endif // VT_BUTTON_ITEM_H
