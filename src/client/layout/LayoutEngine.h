// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LayoutEngine.h — Manages pages and the active page for the POS UI.

#ifndef VT_LAYOUT_ENGINE_H
#define VT_LAYOUT_ENGINE_H

#include "PageWidget.h"
#include "ActionButtonElement.h"

#include <QGraphicsScene>
#include <QHash>
#include <QObject>

namespace vt {

/// The LayoutEngine owns all pages and controls which page is currently
/// displayed in the shared QGraphicsScene.
///
/// Typical usage:
/// @code
///   LayoutEngine engine(&scene);
///   auto *page = engine.createPage("main");
///   page->addButton("btn_press", 760, 480, 400, 120, "PRESS");
///   engine.showPage("main");
/// @endcode
class LayoutEngine : public QObject {
    Q_OBJECT
public:
    explicit LayoutEngine(QGraphicsScene *scene, QObject *parent = nullptr);
    ~LayoutEngine() override = default;

    // ── Page management ─────────────────────────────────────────────────

    /// Create a new page.  Returns nullptr if a page with that name exists.
    PageWidget *createPage(const QString &name);

    /// Remove and delete a page.  Returns true if found.  Cannot remove
    /// the active page; switch away first.
    bool removePage(const QString &name);

    /// Rename a page.  Returns true on success.
    bool renamePage(const QString &oldName, const QString &newName);

    /// Remove all pages (detaches active page first).
    void clearAll();

    /// Get a page by name (nullptr if not found).
    PageWidget *page(const QString &name) const;

    /// Names of all pages.
    QStringList pageNames() const;

    /// Names of system pages only.
    QStringList systemPageNames() const;

    /// Names of user (non-system) pages only.
    QStringList userPageNames() const;

    // ── Navigation ──────────────────────────────────────────────────────

    /// Show the named page (detaches the previous one).
    bool showPage(const QString &name);

    /// Currently active page (may be nullptr if none shown yet).
    PageWidget *activePage() const { return m_activePage; }

    /// Name of the active page (empty string if none).
    QString activePageName() const;

signals:
    /// Emitted whenever the active page changes.
    void pageChanged(const QString &newPageName);

    /// Forwarded: any button on any page was clicked.
    void buttonClicked(const QString &pageName, const QString &elementId);

    /// Forwarded: a keypad button was pressed on a page.
    void keypadPressed(const QString &pageName, const QString &value);

    /// Forwarded: an action button was triggered on a page.
    void actionTriggered(const QString &pageName, vt::ActionType action);

private:
    QGraphicsScene *m_scene = nullptr;
    QHash<QString, PageWidget *> m_pages;
    PageWidget     *m_activePage = nullptr;
};

} // namespace vt

#endif // VT_LAYOUT_ENGINE_H
