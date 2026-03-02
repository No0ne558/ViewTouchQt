// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PinEntryElement.h — PIN entry display field.

#ifndef VT_PIN_ENTRY_ELEMENT_H
#define VT_PIN_ENTRY_ELEMENT_H

#include "UiElement.h"

namespace vt {

/// A PIN entry field that displays entered digits as masked dots (or clear text).
/// Accepts input from KeypadButtonElements and physical keyboard.
class PinEntryElement : public UiElement {
    Q_OBJECT
public:
    explicit PinEntryElement(const QString &id,
                             qreal x, qreal y, qreal w, qreal h,
                             QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// Append a character to the current PIN buffer.
    void appendChar(const QChar &ch);

    /// Remove the last character (backspace).
    void backspace();

    /// Clear the entire PIN buffer.
    void clearPin();

    /// Get the current PIN text.
    const QString &pinText() const { return m_pin; }

    /// Whether to mask the PIN (show dots instead of digits). Default: true.
    bool masked() const { return m_masked; }
    void setMasked(bool on) { m_masked = on; update(); }

    /// Maximum number of characters accepted (0 = unlimited). Default: 8.
    int maxLength() const { return m_maxLength; }
    void setMaxLength(int n) { m_maxLength = n; }

signals:
    /// Emitted whenever the PIN text changes.
    void pinChanged(const QString &pin);

    /// Emitted when Enter/Return is pressed on this element.
    void enterPressed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString m_pin;
    bool    m_masked   = true;
    int     m_maxLength = 8;
};

} // namespace vt

#endif // VT_PIN_ENTRY_ELEMENT_H
