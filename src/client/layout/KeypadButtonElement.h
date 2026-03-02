// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/KeypadButtonElement.h — Keypad button that sends characters to a PinEntry.

#ifndef VT_KEYPAD_BUTTON_ELEMENT_H
#define VT_KEYPAD_BUTTON_ELEMENT_H

#include "UiElement.h"
#include <QTimer>

namespace vt {

/// A button that, when pressed, sends its configured character(s) to the
/// PinEntryElement on the same page.  The button's label text is displayed
/// on screen, and the `keyValue` property determines what character is sent.
///
/// Special keyValue values:
///   - "BACK"  — sends a backspace to the PinEntry
///   - "CLEAR" — clears the PinEntry
///   - Any other string — each character is appended to the PinEntry
class KeypadButtonElement : public UiElement {
    Q_OBJECT
public:
    explicit KeypadButtonElement(const QString &id,
                                 qreal x, qreal y, qreal w, qreal h,
                                 QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// The value sent to the PinEntry when this button is pressed.
    /// Defaults to the label text if not explicitly set.
    const QString &keyValue() const { return m_keyValue; }
    void setKeyValue(const QString &val) { m_keyValue = val; }

    /// Colours for idle and active (flash) states.
    void setActiveColor(const QColor &c) { m_activeColor = c; }
    const QColor &activeColor() const { return m_activeColor; }

    /// Override to keep the current display colour in sync.
    void setBgColor(const QColor &c) override;

signals:
    /// Emitted when the keypad button is pressed.
    /// The value is the keyValue (e.g. "5", "BACK", "CLEAR").
    void keyPressed(const QString &value);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    void resetColor();

private:
    QString m_keyValue;
    QColor  m_currentColor;
    QColor  m_activeColor = QColor(80, 180, 255);
    QTimer  m_flashTimer;
    static constexpr int kFlashDurationMs = 150;
};

} // namespace vt

#endif // VT_KEYPAD_BUTTON_ELEMENT_H
