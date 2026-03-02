// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/EditorOverlay.h — Visual editor overlay for drag-and-drop layout editing.

#ifndef VT_EDITOR_OVERLAY_H
#define VT_EDITOR_OVERLAY_H

#include "../layout/UiElement.h"
#include "../layout/LayoutEngine.h"

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QToolBar>
#include <QAction>
#include <QList>

namespace vt {

/// Small square handle for resizing an element from a corner or edge.
class ResizeHandle : public QGraphicsRectItem {
public:
    enum Position { TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left };

    ResizeHandle(Position pos, QGraphicsItem *parent = nullptr);

    Position handlePosition() const { return m_pos; }

    /// Update handle placement given the parent element's bounding rect.
    void reposition(const QRectF &parentRect);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Position m_pos;
    QPointF  m_pressPos;
    QRectF   m_startRect;
    QPointF  m_startScenePos;
    static constexpr qreal kHandleSize = 14.0;
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

signals:
    void editModeChanged(bool on);
    void selectionChanged(UiElement *elem);
    /// Request to open property editor for the given element.
    void editPropertiesRequested(UiElement *elem);

private:
    void installEventFilter();
    void removeEventFilter();

    /// Scene-level event filter to intercept clicks on elements.
    bool eventFilter(QObject *watched, QEvent *event) override;

    void updateHandles();
    void hideHandles();
    void showToolbar();
    void hideToolbar();

    void createSelectionRect();
    void createResizeHandles();

public:
    /// Snap a position to the grid (optional, 10px).
    static QPointF snapToGrid(const QPointF &pos, qreal grid = 10.0);

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

    // Toolbar
    QGraphicsProxyWidget *m_toolbarProxy = nullptr;
    QToolBar             *m_toolbar      = nullptr;

    int m_nextId = 1;  // auto-increment for new element IDs
};

} // namespace vt

#endif // VT_EDITOR_OVERLAY_H
