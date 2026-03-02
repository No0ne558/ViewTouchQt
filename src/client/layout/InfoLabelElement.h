// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/InfoLabelElement.h — Read-only label that shows build/version info.

#ifndef VT_INFO_LABEL_ELEMENT_H
#define VT_INFO_LABEL_ELEMENT_H

#include "UiElement.h"

namespace vt {

/// A non-interactive label that automatically displays program version
/// information.  The text is set at construction time and refreshed
/// whenever the element is attached to a scene.
class InfoLabelElement : public UiElement {
    Q_OBJECT
public:
    explicit InfoLabelElement(const QString &id,
                              qreal x, qreal y, qreal w, qreal h,
                              QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Refresh the label text with current version info.
    void refreshVersionText();
};

} // namespace vt

#endif // VT_INFO_LABEL_ELEMENT_H
