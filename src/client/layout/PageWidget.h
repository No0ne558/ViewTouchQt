// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PageWidget.h — A named page that owns a set of UI elements.

#ifndef VT_PAGE_WIDGET_H
#define VT_PAGE_WIDGET_H

#include "ButtonElement.h"
#include "UiElement.h"
#include "LoginEntryField.h"
#include "KeyboardButton.h"

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

    /// The name of the page this page inherits elements from (empty = none).
    /// Only elements marked as "inheritable" on the parent page are shown.
    const QString &inheritFrom() const { return m_inheritFrom; }
    void setInheritFrom(const QString &pageName) { m_inheritFrom = pageName; }

    // ── Element management ──────────────────────────────────────────────

    /// Create and add a button.  Returns a non-owning pointer.
    ButtonElement *addButton(const QString &id, qreal x, qreal y,
                             qreal w, qreal h, const QString &label);
    /// Create and add an Image button (image-only). Returns non-owning pointer.
    ButtonElement *addImageButton(const QString &id, qreal x, qreal y,
                                  qreal w, qreal h, const QString &imagePath = QString());

    /// Create and add a login button (special button type that triggers login).
    ButtonElement *addLoginButton(const QString &id, qreal x, qreal y,
                                  qreal w, qreal h, const QString &label = QString());

    /// Create and add a login entry field (masked numeric input).
    LoginEntryField *addLoginEntryField(const QString &id, qreal x, qreal y,
                                        qreal w, qreal h, int maxLength = 9);

    /// Create and add an on-screen keyboard button. These buttons are local-only
    /// by default and emit `virtualKeyPressed` rather than `buttonClicked`.
    KeyboardButton *addKeyboardButton(const QString &id, qreal x, qreal y,
                                      qreal w, qreal h, const QString &assignedKey);

    /// Create and add a label.
    // Labels, panels and other element types were removed; only buttons remain.

    /// Remove an element by id.  Returns true if found.
    bool removeElement(const QString &id);

    /// Replace an element with a new one of a different type, preserving
    /// common properties (position, size, colours, label, font, radius).
    /// Returns the new element, or nullptr if the id was not found.
    UiElement *replaceElementType(const QString &id, ElementType newType);

    /// Look up an element by id (nullptr if not found).
    UiElement *element(const QString &id) const;

    /// Clone an existing element (copy visual properties) and return the new element.
    /// If newId is empty a unique id will be generated. Returns nullptr on failure.
    UiElement *cloneElement(const QString &srcId, const QString &newId = QString());

    /// All elements on this page.
    QList<UiElement *> elements() const { return m_elements.values(); }

    // ── Scene integration ───────────────────────────────────────────────

    /// Attach all elements to the given scene (called when page becomes active).
    void attachToScene(QGraphicsScene *scene);

    /// Detach all elements from the current scene (called on page switch).
    void detachFromScene();

    /// Default input element management: the element id of the page's
    /// preferred input field. If empty, the first LoginEntryField found
    /// on the page will be used as the default.
    void setDefaultInputElementId(const QString &id) { m_defaultInputId = id; }
    QString defaultInputElementId() const { return m_defaultInputId; }
    LoginEntryField *defaultInputField() const;

signals:
    /// Forwarded from any ButtonElement on this page.
    void buttonClicked(const QString &elementId);

    /// Emitted when a `LoginEntryField` on this page is submitted (e.g. Enter pressed).
    void loginFieldSubmitted(const QString &elementId, const QString &value);

    // Only button clicks are forwarded now.

private:
    void registerElement(UiElement *elem);

    // Receive virtual key presses from KeyboardButton instances and route
    // them to the currently focused LoginEntryField in the scene.
    void onVirtualKeyPressed(const QString &key);

    QString m_name;
    bool    m_systemPage = false;
    QString m_inheritFrom;
    QHash<QString, UiElement *> m_elements;
    QGraphicsScene *m_currentScene = nullptr;
    QString m_defaultInputId;
    int m_autoIdCounter = 1;
};

} // namespace vt

#endif // VT_PAGE_WIDGET_H
