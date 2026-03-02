// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PageWidget.h — A named page that owns a set of UI elements.

#ifndef VT_PAGE_WIDGET_H
#define VT_PAGE_WIDGET_H

#include "ButtonElement.h"
#include "LabelElement.h"
#include "PanelElement.h"
#include "PinEntryElement.h"
#include "KeypadButtonElement.h"
#include "ActionButtonElement.h"

#include <QGraphicsScene>
#include <QHash>
#include <QObject>

namespace vt {

/// A page in the POS UI.  Each page has its own collection of UiElements
/// that are added to / removed from a shared QGraphicsScene when the page
/// becomes active / inactive.
class PageWidget : public QObject {
    Q_OBJECT
public:
    explicit PageWidget(const QString &name, QObject *parent = nullptr);
    ~PageWidget() override;

    const QString &name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    /// Whether this is a system page (Login, Tables, Order, etc.).
    /// System pages control POS flow and are created automatically.
    bool isSystemPage() const { return m_systemPage; }
    void setSystemPage(bool on) { m_systemPage = on; }

    // ── Element management ──────────────────────────────────────────────

    /// Create and add a button.  Returns a non-owning pointer.
    ButtonElement *addButton(const QString &id, qreal x, qreal y,
                             qreal w, qreal h, const QString &label);

    /// Create and add a label.
    LabelElement *addLabel(const QString &id, qreal x, qreal y,
                           qreal w, qreal h, const QString &text);

    /// Create and add a panel.
    PanelElement *addPanel(const QString &id, qreal x, qreal y,
                           qreal w, qreal h);

    /// Create and add a PIN entry field.
    PinEntryElement *addPinEntry(const QString &id, qreal x, qreal y,
                                 qreal w, qreal h);

    /// Create and add a keypad button.
    KeypadButtonElement *addKeypadButton(const QString &id, qreal x, qreal y,
                                         qreal w, qreal h, const QString &label);

    /// Create and add an action button (Login, Dine-In, To-Go).
    ActionButtonElement *addActionButton(const QString &id, qreal x, qreal y,
                                         qreal w, qreal h, const QString &label,
                                         ActionType action);

    /// Remove an element by id.  Returns true if found.
    bool removeElement(const QString &id);

    /// Look up an element by id (nullptr if not found).
    UiElement *element(const QString &id) const;

    /// All elements on this page.
    QList<UiElement *> elements() const { return m_elements.values(); }

    // ── Scene integration ───────────────────────────────────────────────

    /// Attach all elements to the given scene (called when page becomes active).
    void attachToScene(QGraphicsScene *scene);

    /// Detach all elements from the current scene (called on page switch).
    void detachFromScene();

signals:
    /// Forwarded from any ButtonElement on this page.
    void buttonClicked(const QString &elementId);

    /// Forwarded from any KeypadButtonElement on this page.
    void keypadPressed(const QString &value);

    /// Forwarded from any ActionButtonElement on this page.
    void actionTriggered(vt::ActionType action);

private:
    void registerElement(UiElement *elem);

    QString m_name;
    bool    m_systemPage = false;
    QHash<QString, UiElement *> m_elements;
    QGraphicsScene *m_currentScene = nullptr;
};

} // namespace vt

#endif // VT_PAGE_WIDGET_H
