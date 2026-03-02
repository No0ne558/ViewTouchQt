// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageTabBar.h — Floating draggable page list panel for the editor.

#ifndef VT_PAGE_TAB_BAR_H
#define VT_PAGE_TAB_BAR_H

#include "../layout/LayoutEngine.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QList>

namespace vt {

class ToolbarDragHandle;  // from EditorOverlay.h

/// A floating, draggable panel that lists all pages with clickable buttons.
/// Toggled by the "Page List" button in the editor toolbar.
class PageTabBar : public QObject {
    Q_OBJECT
public:
    explicit PageTabBar(LayoutEngine *engine, QGraphicsScene *scene,
                        QObject *parent = nullptr);
    ~PageTabBar() override;

    /// Rebuild the page buttons from the current engine page list.
    void refresh();

    /// Toggle visibility. If visible and refresh is needed, rebuilds.
    void setVisible(bool visible);
    bool isVisible() const { return m_container != nullptr; }

    /// Toggle: show if hidden, hide if shown.
    void toggle();

    /// Returns the container graphics item (for event filter hit-testing).
    QGraphicsItem *containerItem() const { return m_container; }

signals:
    /// Emitted when user clicks a page tab.
    void pageSelected(const QString &pageName);
    /// Emitted when user clicks "Manage Pages..." inside the panel.
    void manageRequested();

private:
    void buildWidget();
    void destroyWidget();

    LayoutEngine   *m_engine    = nullptr;
    QGraphicsScene *m_scene     = nullptr;

    QGraphicsRectItem    *m_container = nullptr;   // invisible movable parent
    ToolbarDragHandle    *m_grip      = nullptr;
    QWidget              *m_panel     = nullptr;
    QGraphicsProxyWidget *m_proxy     = nullptr;
    QList<QPushButton *>  m_tabs;
};

} // namespace vt

#endif // VT_PAGE_TAB_BAR_H
