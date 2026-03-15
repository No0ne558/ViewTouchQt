// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VT_GHOST_ITEM_H
#define VT_GHOST_ITEM_H

#include <QGraphicsItem>
#include <QRectF>
#include <QString>

namespace vt {

class GhostItem : public QGraphicsItem {
public:
    explicit GhostItem(const QSizeF &size, const QString &label = QString(), QGraphicsItem *parent = nullptr);
    ~GhostItem() override = default;

    void setSize(const QSizeF &size);
    void setLabel(const QString &label);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    QRectF m_rect;
    QString m_label;
};

} // namespace vt

#endif // VT_GHOST_ITEM_H
