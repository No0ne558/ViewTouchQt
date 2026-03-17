// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LoginEntryField.h — Masked numeric entry field for login pages.

#ifndef VT_LOGIN_ENTRY_FIELD_H
#define VT_LOGIN_ENTRY_FIELD_H

#include "UiElement.h"
#include <QString>

namespace vt {

class LoginEntryField : public UiElement {
    Q_OBJECT
public:
    explicit LoginEntryField(const QString &id,
                             qreal x, qreal y, qreal w, qreal h,
                             QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // Programmatic input from virtual keyboard buttons
    void appendKey(const QString &k);
    void backspace();
    void clear();

    QString value() const { return m_value; }
    void setMaxLength(int n) { m_maxLength = n; }
    int maxLength() const { return m_maxLength; }
    void submit();
    

signals:
    void valueChanged(const QString &value);
    void submitted(const QString &value);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString m_value;
    int m_maxLength = 9;
    QChar m_maskChar = QChar('*');
};

} // namespace vt

#endif // VT_LOGIN_ENTRY_FIELD_H
