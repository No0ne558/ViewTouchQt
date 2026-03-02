// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/EditorOverlay.h — Visual editor overlay for drag-and-drop layout editing.

#ifndef VT_EDITOR_OVERLAY_H
#define VT_EDITOR_OVERLAY_H

#include "../layout/UiElement.h"
#include "../layout/LayoutEngine.h"
#include "PageTabBar.h"

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QToolBar>
#include <QAction>
#include <QList>

namespace vt {

class EditorOverlay;  // forward declaration

/// Small square handle for resizing an element from a corner or edge.
class ResizeHandle : public QGraphicsRectItem {
public:
    enum Position { TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

    ResizeHandle(Position pos, EditorOverlay *overlay, QGraphicsItem *parent = nullptr);

    Position handlePosition() const { return m_pos; }

    /// Update handle placement given the parent element's bounding rect.
    void reposition(const QRectF &parentRect);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Position m_pos;
    EditorOverlay *m_overlay = nullptr;
    QPointF  m_pressPos;
    QRectF   m_startRect;
    QPointF  m_startScenePos;
    static constexpr qreal kHandleSize = 14.0;
};

/// A small grip area that lets the user drag the floating toolbar around.
class ToolbarDragHandle : public QGraphicsRectItem {
public:
    explicit ToolbarDragHandle(qreal w, qreal h, QGraphicsItem *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QPointF m_dragStart;
    QPointF m_posStart;
};

/// The EditorOverlay manages the visual editing state.
/// When active, elements become draggable, show selection outlines
/// and resize handles, and a toolbar allows adding/removing elements.
class EditorOverlay : public QObject {
    Q_OBJECT
public:
    explicit EditorOverlay(LayoutEngine *engine, QGraphicsScene *scene,
                           QObject *parent = nullptr);
    ~EditorOverlay() override;

    /// Toggle editing mode on/off.
    void setEditMode(bool on);
    bool isEditMode() const { return m_editMode; }

    /// Currently selected element (nullptr if none).
    UiElement *selectedElement() const { return m_selected; }

    /// Select an element (shows handles, outline).
    void selectElement(UiElement *elem);

    /// Deselect current element.
    void deselectAll();

    /// Remove the currently selected element.
    void deleteSelected();

    /// Add a new element of the given type at the center of the scene.
    void addElement(ElementType type);

    /// Access the page tab bar (for external refresh calls).
    PageTabBar *pageTabBar() { return m_pageTabBar; }

signals:
    void editModeChanged(bool on);
    void selectionChanged(UiElement *elem);
    /// Request to open property editor for the given element.
    void editPropertiesRequested(UiElement *elem);
    /// Request to open the page manager dialog.
    void pageManagerRequested();

public:
    /// Snap a position to the grid (optional, 10px).
    static QPointF snapToGrid(const QPointF &pos, qreal grid = 10.0);

    /// Refresh selection rect + resize handle positions (called by ResizeHandle too).
    void updateHandles();

private:
    void installEventFilter();
    void removeEventFilter();

    /// Scene-level event filter to intercept clicks on elements.
    bool eventFilter(QObject *watched, QEvent *event) override;

    void hideHandles();
    void showToolbar();
    void hideToolbar();

    void createSelectionRect();
    void createResizeHandles();

private:

    LayoutEngine   *m_engine     = nullptr;
    QGraphicsScene *m_scene      = nullptr;
    bool            m_editMode   = false;

    UiElement *m_selected = nullptr;

    // Selection decoration
    QGraphicsRectItem *m_selectionRect = nullptr;
    QList<ResizeHandle *> m_handles;

    // Dragging state
    bool    m_dragging       = false;
    QPointF m_dragStartScene;
    QPointF m_dragStartPos;

    // Toolbar (movable container: invisible rect → drag handle + toolbar proxy)
    QGraphicsRectItem    *m_toolbarGroup = nullptr;
    QGraphicsProxyWidget *m_toolbarProxy = nullptr;
    QToolBar             *m_toolbar      = nullptr;
    ToolbarDragHandle    *m_dragHandle   = nullptr;

    // Page list panel (floating, toggled by toolbar button)
    PageTabBar *m_pageTabBar = nullptr;

    int m_nextId = 1;  // auto-increment for new element IDs
};

} // namespace vt

#endif // VT_EDITOR_OVERLAY_H
