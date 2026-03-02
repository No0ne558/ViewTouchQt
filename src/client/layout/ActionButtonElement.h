// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/ActionButtonElement.h — Navigation action button (Login, Dine-In, To-Go).

#ifndef VT_ACTION_BUTTON_ELEMENT_H
#define VT_ACTION_BUTTON_ELEMENT_H

#include "UiElement.h"
#include <QTimer>

namespace vt {

/// The action a button performs when pressed.
enum class ActionType {
    Login,    // Navigate to the Tables page
    DineIn,   // Navigate to the Order page
    ToGo,     // Navigate to the Order page
    Logout,   // Navigate back to the Login page
};

/// A button that performs a POS navigation action.
/// On the Login page these require a non-empty PIN to proceed.
/// The `actionTriggered` signal carries the action type so the
/// application layer can perform the actual navigation.
class ActionButtonElement : public UiElement {
    Q_OBJECT
public:
    explicit ActionButtonElement(const QString &id,
                                 qreal x, qreal y, qreal w, qreal h,
                                 QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    /// The action this button performs.
    ActionType actionType() const { return m_action; }
    void setActionType(ActionType action) { m_action = action; }

    /// Colours for idle and active (flash) states.
    void setActiveColor(const QColor &c) { m_activeColor = c; }
    const QColor &activeColor() const { return m_activeColor; }

    /// Override to keep the current display colour in sync.
    void setBgColor(const QColor &c) override;

signals:
    /// Emitted when the button is pressed.
    void actionTriggered(vt::ActionType action);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    void resetColor();

private:
    ActionType m_action = ActionType::Login;
    QColor  m_currentColor;
    QColor  m_activeColor = QColor(255, 220, 50);
    QTimer  m_flashTimer;
    static constexpr int kFlashDurationMs = 200;
};

} // namespace vt

#endif // VT_ACTION_BUTTON_ELEMENT_H
