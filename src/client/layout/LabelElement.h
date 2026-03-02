// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LabelElement.h — Static text display.

#ifndef VT_LABEL_ELEMENT_H
#define VT_LABEL_ELEMENT_H

#include "UiElement.h"

namespace vt {

class LabelElement : public UiElement {
    Q_OBJECT
public:
    explicit LabelElement(const QString &id,
                          qreal x, qreal y, qreal w, qreal h,
                          QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Horizontal alignment (default: AlignCenter).
    void setAlignment(Qt::Alignment align) { m_alignment = align; update(); }
    Qt::Alignment alignment() const { return m_alignment; }

    /// Whether to draw the background rect (default: false = transparent).
    void setDrawBackground(bool on) { m_drawBg = on; update(); }
    bool drawBackground() const { return m_drawBg; }

private:
    Qt::Alignment m_alignment = Qt::AlignCenter;
    bool m_drawBg = false;
};

} // namespace vt

#endif // VT_LABEL_ELEMENT_H
