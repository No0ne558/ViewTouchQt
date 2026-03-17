// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/KeyboardButton.h — On-screen keyboard key element.

#ifndef VT_KEYBOARD_BUTTON_H
#define VT_KEYBOARD_BUTTON_H

#include "ButtonElement.h"
#include <QString>

namespace vt {

class KeyboardButton : public ButtonElement {
    Q_OBJECT
public:
    explicit KeyboardButton(const QString &id,
                            qreal x, qreal y, qreal w, qreal h,
                            const QString &assignedKey = QString(),
                            QGraphicsItem *parent = nullptr);

    const QString &assignedKey() const { return m_assignedKey; }
    void setAssignedKey(const QString &k);

signals:
    void virtualKeyPressed(const QString &key);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString m_assignedKey;
};

} // namespace vt

#endif // VT_KEYBOARD_BUTTON_H
