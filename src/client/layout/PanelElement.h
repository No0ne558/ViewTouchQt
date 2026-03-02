// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PanelElement.h — Background container / divider.

#ifndef VT_PANEL_ELEMENT_H
#define VT_PANEL_ELEMENT_H

#include "UiElement.h"

namespace vt {

/// A non-interactive rectangular panel used as a background container or
/// visual divider to group other elements.
class PanelElement : public UiElement {
    Q_OBJECT
public:
    explicit PanelElement(const QString &id,
                          qreal x, qreal y, qreal w, qreal h,
                          QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Border colour (default: transparent = no border).
    void setBorderColor(const QColor &c) { m_borderColor = c; update(); }
    const QColor &borderColor() const { return m_borderColor; }

    void setBorderWidth(qreal w) { m_borderWidth = w; update(); }
    qreal borderWidth() const { return m_borderWidth; }

private:
    QColor m_borderColor = Qt::transparent;
    qreal  m_borderWidth = 0;
};

} // namespace vt

#endif // VT_PANEL_ELEMENT_H
