// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/EditorOverlay.cpp

#include "EditorOverlay.h"
#include "../layout/ButtonElement.h"
#include "../layout/PageWidget.h"
#include "GhostItem.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPen>
#include <QTimer>
#include <QToolButton>

namespace vt {

// ═════════════════════════════════════════════════════════════════════════════
// ResizeHandle
// ═════════════════════════════════════════════════════════════════════════════

ResizeHandle::ResizeHandle(Position pos, EditorOverlay *overlay, QGraphicsItem *parent)
    : QGraphicsRectItem(-kHandleSize / 2, -kHandleSize / 2,
                        kHandleSize, kHandleSize, parent)
    , m_pos(pos)
    , m_overlay(overlay)
{
    setBrush(QColor(0, 120, 255));
    setPen(QPen(Qt::white, 1));
    setZValue(10000);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    // Set appropriate cursor shape
    switch (pos) {
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    }
}

void ResizeHandle::reposition(const QRectF &r)
{
    switch (m_pos) {
    case TopLeft:     setPos(r.topLeft()); break;
    case Top:         setPos(r.center().x(), r.top()); break;
    case TopRight:    setPos(r.topRight()); break;
    case Right:       setPos(r.right(), r.center().y()); break;
    case BottomRight: setPos(r.bottomRight()); break;
    case Bottom:      setPos(r.center().x(), r.bottom()); break;
    case BottomLeft:  setPos(r.bottomLeft()); break;
    case Left:        setPos(r.left(), r.center().y()); break;
    }
}

void ResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_pressPos = event->scenePos();
    auto *parent = dynamic_cast<UiElement *>(parentItem());
    if (parent) {
        m_startRect = QRectF(parent->pos(), QSizeF(parent->elementW(), parent->elementH()));
    }
    m_startScenePos = event->scenePos();
    event->accept();
}

void ResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    auto *elem = dynamic_cast<UiElement *>(parentItem());
    if (!elem) return;

    QPointF delta = event->scenePos() - m_startScenePos;
    QRectF r = m_startRect;

    constexpr qreal kMinSize = 40.0;

    switch (m_pos) {
    case TopLeft:
        r.setTopLeft(r.topLeft() + delta);
        break;
    case Top:
        r.setTop(r.top() + delta.y());
        break;
    case TopRight:
        r.setTopRight(r.topRight() + delta);
        break;
    case Right:
        r.setRight(r.right() + delta.x());
        break;
    case BottomRight:
        r.setBottomRight(r.bottomRight() + delta);
        break;
    case Bottom:
        r.setBottom(r.bottom() + delta.y());
        break;
    case BottomLeft:
        r.setBottomLeft(r.bottomLeft() + delta);
        break;
    case Left:
        r.setLeft(r.left() + delta.x());
        break;
    }

    // Enforce minimum size
    if (r.width() < kMinSize) r.setWidth(kMinSize);
    if (r.height() < kMinSize) r.setHeight(kMinSize);

    // Snap to grid
    QPointF snapped = EditorOverlay::snapToGrid(r.topLeft());
    r.moveTopLeft(snapped);

    elem->setPos(r.topLeft());
    elem->resizeTo(r.width(), r.height());

    // Update all sibling handles and selection rect in real-time
    if (m_overlay)
        m_overlay->updateHandles();
    event->accept();
}

void ResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    // Trigger handle repositioning by signalling the parent
    auto *elem = dynamic_cast<UiElement *>(parentItem());
    if (elem) {
        emit elem->elementResized(elem->elementW(), elem->elementH());
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// EditorOverlay
// ═════════════════════════════════════════════════════════════════════════════

EditorOverlay::EditorOverlay(LayoutEngine *engine, QGraphicsScene *scene,
                             QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_scene(scene)
{
    m_pageTabBar = new PageTabBar(engine, scene, this);

    // When user clicks a tab, deselect (elements may belong to old page)
    connect(m_pageTabBar, &PageTabBar::pageSelected, this, [this]() {
        deselectAll();
    });

    // Forward "Manage Pages..." click to the main window
    connect(m_pageTabBar, &PageTabBar::manageRequested, this, [this]() {
        emit pageManagerRequested();
    });
}

EditorOverlay::~EditorOverlay()
{
    hideHandles();
    hideToolbar();
    m_pageTabBar->setVisible(false);
}

// ── Edit mode ───────────────────────────────────────────────────────────────

void EditorOverlay::setEditMode(bool on)
{
    if (m_editMode == on)
        return;

    m_editMode = on;

    if (on) {
        installEventFilter();
        showToolbar();
        qDebug() << "[editor] Edit mode ON";
    } else {
        deselectAll();
        removeEventFilter();
        hideToolbar();
        m_pageTabBar->setVisible(false);   // hide panel when leaving edit mode
        qDebug() << "[editor] Edit mode OFF";
    }

    emit editModeChanged(on);
}

// ── Selection ───────────────────────────────────────────────────────────────

void EditorOverlay::selectElement(UiElement *elem)
{
    if (m_selection.size() == 1 && m_selection.first() == elem)
        return;

    deselectAll();

    if (!elem)
        return;

    m_selection.append(elem);
    m_selected = elem;

    addSelectionDecoration(elem);
    emit selectionChanged(elem);
}

void EditorOverlay::toggleElementInSelection(UiElement *elem)
{
    if (!elem) return;

    if (m_selection.contains(elem)) {
        // Remove from selection
        removeSelectionDecoration(elem);
        m_selection.removeOne(elem);
    } else {
        // Add to selection
        m_selection.append(elem);
        addSelectionDecoration(elem);
    }

    m_selected = m_selection.isEmpty() ? nullptr : m_selection.first();

    // Resize handles only shown for single selection
    if (m_selection.size() != 1) {
        // Strip resize handles from all decorated elements
        for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
            qDeleteAll(it->handles);
            it->handles.clear();
        }
    } else if (m_selection.size() == 1) {
        // Re-add handles for the single selected element
        removeSelectionDecoration(m_selection.first());
        addSelectionDecoration(m_selection.first());
    }

    emit selectionChanged(m_selected);
}

void EditorOverlay::deselectAll()
{
    hideHandles();
    m_selection.clear();
    m_selected = nullptr;
    emit selectionChanged(nullptr);
}

void EditorOverlay::deleteSelected()
{
    if (m_selection.isEmpty() || !m_engine->activePage())
        return;

    QStringList ids;
    for (auto *elem : m_selection)
        ids.append(elem->elementId());

    deselectAll();

    for (const QString &id : ids)
        m_engine->activePage()->removeElement(id);
}

// ── Add element ─────────────────────────────────────────────────────────────

void EditorOverlay::addElement(ElementType type)
{
    auto *pg = m_engine->activePage();
    if (!pg) return;

    // Only allow adding Button elements in the simplified editor.
    QString prefix = QStringLiteral("btn_");
    QString id;
    do {
        id = prefix + QString::number(m_nextId++);
    } while (pg->element(id));

    constexpr qreal cx = 960.0;
    constexpr qreal cy = 540.0;

    auto *btn = pg->addButton(id, cx - 100, cy - 30, 200, 60, QStringLiteral("New Button"));
    if (btn) {
        btn->setBgColor(QColor(100, 150, 200));
        btn->setTextColor(Qt::white);
        selectElement(btn);
    }
}

// ── Event filter ────────────────────────────────────────────────────────────

void EditorOverlay::installEventFilter()
{
    m_scene->installEventFilter(this);
}

void EditorOverlay::removeEventFilter()
{
    m_scene->removeEventFilter(this);
}

bool EditorOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_editMode)
        return false;

    if (event->type() == QEvent::GraphicsSceneMousePress) {
        auto *me = static_cast<QGraphicsSceneMouseEvent *>(event);

        // Check if we clicked on a resize handle — let it handle itself
        QGraphicsItem *item = m_scene->itemAt(me->scenePos(), QTransform());
        if (auto *handle = dynamic_cast<ResizeHandle *>(item)) {
            return false;  // let handle process
        }

        // Check if click is on the toolbar proxy or drag handle
        if (m_toolbarProxy && m_toolbarProxy->contains(
                m_toolbarProxy->mapFromScene(me->scenePos()))) {
            return false;  // let toolbar process
        }
        if (m_dragHandle && m_dragHandle->contains(
                m_dragHandle->mapFromScene(me->scenePos()))) {
            return false;  // let drag handle process
        }

        // Check if click is on the floating page list panel
        if (m_pageTabBar && m_pageTabBar->isVisible()) {
            QGraphicsItem *topItem = m_scene->itemAt(me->scenePos(), QTransform());
            if (topItem) {
                // Walk up parent chain: if we reach the page tab bar container, let it handle
                QGraphicsItem *check = topItem;
                while (check) {
                    if (check == m_pageTabBar->containerItem()) {
                        return false;  // let page list panel process
                    }
                    check = check->parentItem();
                }
            }
        }

        // Check if we clicked on an element
        auto *elem = dynamic_cast<UiElement *>(item);
        if (!elem) {
            // Maybe clicked on a child item of the element
            if (item && item->parentItem())
                elem = dynamic_cast<UiElement *>(item->parentItem());
        }

        if (elem) {
            if (me->button() == Qt::RightButton) {
                // Right-click: select and open property editor immediately
                if (!m_selection.contains(elem))
                    selectElement(elem);
                emit editPropertiesRequested(elem);
            } else if (me->button() == Qt::LeftButton) {
                bool ctrl = (me->modifiers() & Qt::ControlModifier);

                if (ctrl) {
                    // Ctrl+click: toggle in multi-selection
                    toggleElementInSelection(elem);
                } else {
                    // Plain click: if element is already in a multi-selection,
                    // start drag of the whole group; otherwise single-select.
                    if (!m_selection.contains(elem))
                        selectElement(elem);
                }

                // Start drag for all selected elements
                if (!m_selection.isEmpty()) {
                    m_dragging = true;
                    m_dragStartScene = me->scenePos();
                    m_dragStartPositions.clear();
                    for (auto *sel : m_selection)
                        m_dragStartPositions.insert(sel, sel->pos());
                    m_dragStartPos = m_selection.first()->pos();
                    // Create ghost overlay to avoid moving heavy items each frame
                    if (m_enableGhostDrag) {
                        createGhostForSelection(m_selection);
                        m_usingGhostDuringDrag = (m_dragGhost != nullptr);
                    }
                }
            }
            return true;  // consume event — don't trigger button clicks
        } else {
            // Clicked on empty space
            if (me->button() == Qt::LeftButton) {
                bool ctrl = (me->modifiers() & Qt::ControlModifier);
                if (!ctrl)
                    deselectAll();
                // Start rubber-band selection
                beginRubberBand(me->scenePos());
            } else {
                deselectAll();
            }
            return true;
        }
    }

    if (event->type() == QEvent::GraphicsSceneMouseMove) {
        auto *me = static_cast<QGraphicsSceneMouseEvent *>(event);

        if (m_rubberBanding) {
            updateRubberBand(me->scenePos());
            return true;
        }

        if (m_dragging && !m_selection.isEmpty()) {
            QPointF delta = me->scenePos() - m_dragStartScene;
            if (m_usingGhostDuringDrag && m_dragGhost) {
                QPointF newTopLeft = m_dragGhostStartRect.topLeft() + delta;
                newTopLeft = snapToGrid(newTopLeft);
                m_dragGhost->setPos(newTopLeft);
            } else {
                for (auto *sel : m_selection) {
                    QPointF startPos = m_dragStartPositions.value(sel);
                    sel->setPos(snapToGrid(startPos + delta));
                }
                updateHandles();
            }
            return true;
        }
    }

    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        if (m_rubberBanding) {
            endRubberBand();
            return true;
        }
        if (m_dragging) {
            auto *me = static_cast<QGraphicsSceneMouseEvent *>(event);
            // If using ghost, commit the final position to real items.
            if (m_usingGhostDuringDrag) {
                commitGhostMove(me->scenePos());
            } else {
                m_dragging = false;
                m_dragStartPositions.clear();
                updateHandles();
            }
            return true;
        }
    }

    // Double-click opens property editor
    if (event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        auto *me = static_cast<QGraphicsSceneMouseEvent *>(event);
        QGraphicsItem *item = m_scene->itemAt(me->scenePos(), QTransform());
        auto *elem = dynamic_cast<UiElement *>(item);
        if (!elem && item && item->parentItem())
            elem = dynamic_cast<UiElement *>(item->parentItem());
        if (elem) {
            emit editPropertiesRequested(elem);
            return true;
        }
    }

    // Arrow keys move all selected elements (Shift = 1px fine move)
    if (event->type() == QEvent::KeyPress && !m_selection.isEmpty()) {
        auto *ke = static_cast<QKeyEvent *>(event);
        qreal step = (ke->modifiers() & Qt::ShiftModifier) ? 1.0 : 10.0;
        QPointF delta;
        switch (ke->key()) {
        case Qt::Key_Left:  delta = QPointF(-step, 0); break;
        case Qt::Key_Right: delta = QPointF( step, 0); break;
        case Qt::Key_Up:    delta = QPointF(0, -step); break;
        case Qt::Key_Down:  delta = QPointF(0,  step); break;
        default: break;
        }
        if (!delta.isNull()) {
            for (auto *sel : m_selection)
                sel->setPos(sel->pos() + delta);
            updateHandles();
            return true;
        }

        // W / H resize all selected elements
        {
            constexpr qreal kResizeStep = 10.0;
            constexpr qreal kMinSize    = 40.0;
            bool shrink = (ke->modifiers() & Qt::ShiftModifier);
            if (ke->key() == Qt::Key_W) {
                for (auto *sel : m_selection) {
                    qreal newW = sel->elementW() + (shrink ? -kResizeStep : kResizeStep);
                    if (newW >= kMinSize)
                        sel->resizeTo(newW, sel->elementH());
                }
                updateHandles();
                return true;
            }
            if (ke->key() == Qt::Key_H) {
                for (auto *sel : m_selection) {
                    qreal newH = sel->elementH() + (shrink ? -kResizeStep : kResizeStep);
                    if (newH >= kMinSize)
                        sel->resizeTo(sel->elementW(), newH);
                }
                updateHandles();
                return true;
            }
        }
    }

    return false;
}

// ── Handles / Decoration ────────────────────────────────────────────────────

void EditorOverlay::addSelectionDecoration(UiElement *elem)
{
    if (!elem) return;

    // Remove existing decoration if any
    removeSelectionDecoration(elem);

    ElemDecor decor;

    // Selection rectangle (dashed outline)
    decor.selRect = new QGraphicsRectItem(elem);
    decor.selRect->setPen(QPen(QColor(0, 120, 255), 2, Qt::DashLine));
    decor.selRect->setBrush(Qt::NoBrush);
    decor.selRect->setZValue(9999);

    // Resize handles only for single selection
    if (m_selection.size() == 1 && m_selection.first() == elem) {
        const ResizeHandle::Position positions[] = {
            ResizeHandle::TopLeft, ResizeHandle::Top, ResizeHandle::TopRight,
            ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom,
            ResizeHandle::BottomLeft, ResizeHandle::Left
        };
        for (auto pos : positions) {
            auto *h = new ResizeHandle(pos, this, elem);
            decor.handles.append(h);
        }
    }

    m_decor.insert(elem, decor);

    // Position decorations
    QRectF r = elem->boundingRect();
    decor.selRect->setRect(r.adjusted(-3, -3, 3, 3));
    for (auto *h : decor.handles)
        h->reposition(r);
}

void EditorOverlay::removeSelectionDecoration(UiElement *elem)
{
    if (!elem) return;

    auto it = m_decor.find(elem);
    if (it == m_decor.end()) return;

    delete it->selRect;
    qDeleteAll(it->handles);
    m_decor.erase(it);
}

void EditorOverlay::createSelectionRect()
{
    // No-op: selection rects are now per-element in addSelectionDecoration
}

void EditorOverlay::createResizeHandles()
{
    // No-op: handles are now per-element in addSelectionDecoration
}

void EditorOverlay::updateHandles()
{
    for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
        UiElement *elem = it.key();
        QRectF r = elem->boundingRect();
        if (it->selRect)
            it->selRect->setRect(r.adjusted(-3, -3, 3, 3));
        for (auto *h : it->handles)
            h->reposition(r);
    }
}

// Create a lightweight ghost item that covers the bounding box of the
// current selection. The real items are left untouched until commit.
void EditorOverlay::createGhostForSelection(const QList<UiElement *> &selection)
{
    if (m_dragGhost) {
        m_scene->removeItem(m_dragGhost);
        delete m_dragGhost;
        m_dragGhost = nullptr;
    }

    if (selection.isEmpty())
        return;

    // Compute selection bounding rect in scene coordinates
    QRectF bounds;
    bool first = true;
    for (auto *el : selection) {
        QRectF r(el->pos(), QSizeF(el->elementW(), el->elementH()));
        if (first) { bounds = r; first = false; }
        else bounds = bounds.united(r);
    }

    m_dragGhostStartRect = bounds;

    // Label only for single-element selection (use element label)
    QString label;
    if (selection.size() == 1)
        label = selection.first()->label();

    m_dragGhost = new GhostItem(bounds.size(), label);
    m_dragGhost->setPos(bounds.topLeft());
    m_dragGhost->setZValue(30000);
    m_scene->addItem(m_dragGhost);

    // Hide selection decorations while dragging to avoid per-frame updates
    for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
        if (it->selRect) it->selRect->setVisible(false);
        for (auto *h : it->handles) h->setVisible(false);
    }
}

void EditorOverlay::commitGhostMove(const QPointF & /*scenePos*/)
{
    if (!m_dragGhost) {
        // Nothing to commit
        m_dragging = false;
        m_usingGhostDuringDrag = false;
        m_dragStartPositions.clear();
        updateHandles();
        return;
    }

    // Compute delta from original bounding rect top-left to ghost position
    QPointF delta = m_dragGhost->pos() - m_dragGhostStartRect.topLeft();

    // Apply to real items
    for (auto *sel : m_selection) {
        QPointF startPos = m_dragStartPositions.value(sel);
        sel->setPos(snapToGrid(startPos + delta));
    }

    // Remove ghost
    m_scene->removeItem(m_dragGhost);
    delete m_dragGhost;
    m_dragGhost = nullptr;

    m_usingGhostDuringDrag = false;
    m_dragging = false;
    m_dragStartPositions.clear();

    // Restore selection decorations
    for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
        if (it->selRect) it->selRect->setVisible(true);
        for (auto *h : it->handles) h->setVisible(true);
    }

    updateHandles();
}

void EditorOverlay::cancelGhostMove()
{
    if (m_dragGhost) {
        m_scene->removeItem(m_dragGhost);
        delete m_dragGhost;
        m_dragGhost = nullptr;
    }

    m_usingGhostDuringDrag = false;
    m_dragging = false;
    m_dragStartPositions.clear();

    // Restore decorations
    for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
        if (it->selRect) it->selRect->setVisible(true);
        for (auto *h : it->handles) h->setVisible(true);
    }

    updateHandles();
}

void EditorOverlay::hideHandles()
{
    for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
        delete it->selRect;
        qDeleteAll(it->handles);
    }
    m_decor.clear();

    // Also clean up any rubber-band
    cancelRubberBand();
}

// ── Rubber-band selection ───────────────────────────────────────────────────

void EditorOverlay::beginRubberBand(const QPointF &scenePos)
{
    m_rubberBanding = true;
    m_rubberBandOrigin = scenePos;

    if (!m_rubberBandRect) {
        m_rubberBandRect = new QGraphicsRectItem;
        m_rubberBandRect->setPen(QPen(QColor(0, 120, 255), 1, Qt::DashLine));
        m_rubberBandRect->setBrush(QColor(0, 120, 255, 40));
        m_rubberBandRect->setZValue(19999);
        m_scene->addItem(m_rubberBandRect);
    }
    m_rubberBandRect->setRect(QRectF(scenePos, scenePos).normalized());
    m_rubberBandRect->setVisible(true);
}

void EditorOverlay::updateRubberBand(const QPointF &scenePos)
{
    if (!m_rubberBandRect) return;
    QRectF r(m_rubberBandOrigin, scenePos);
    m_rubberBandRect->setRect(r.normalized());
}

void EditorOverlay::endRubberBand()
{
    if (!m_rubberBandRect) {
        m_rubberBanding = false;
        return;
    }

    QRectF selArea = m_rubberBandRect->rect();
    cancelRubberBand();

    // Only select if the drag area is meaningful (at least a few pixels)
    if (selArea.width() < 5 && selArea.height() < 5)
        return;

    // Find all UiElements whose bounding box intersects the rubber band
    auto *page = m_engine->activePage();
    if (!page) return;

    for (auto *elem : page->elements()) {
        QRectF elemRect(elem->pos(), QSizeF(elem->elementW(), elem->elementH()));
        if (selArea.intersects(elemRect)) {
            if (!m_selection.contains(elem)) {
                m_selection.append(elem);
                addSelectionDecoration(elem);
            }
        }
    }

    // Strip resize handles if multi-selected
    if (m_selection.size() > 1) {
        for (auto it = m_decor.begin(); it != m_decor.end(); ++it) {
            qDeleteAll(it->handles);
            it->handles.clear();
        }
    }

    m_selected = m_selection.isEmpty() ? nullptr : m_selection.first();
    emit selectionChanged(m_selected);
}

void EditorOverlay::cancelRubberBand()
{
    m_rubberBanding = false;
    if (m_rubberBandRect) {
        m_scene->removeItem(m_rubberBandRect);
        delete m_rubberBandRect;
        m_rubberBandRect = nullptr;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// ToolbarDragHandle
// ═════════════════════════════════════════════════════════════════════════════

ToolbarDragHandle::ToolbarDragHandle(qreal w, qreal h, QGraphicsItem *parent)
    : QGraphicsRectItem(0, 0, w, h, parent)
{
    setBrush(Qt::NoBrush);
    setPen(Qt::NoPen);
    setCursor(Qt::OpenHandCursor);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
}

void ToolbarDragHandle::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem * /*option*/,
                              QWidget * /*widget*/)
{
    // Draw 3 horizontal grip lines to hint "drag me"
    painter->setRenderHint(QPainter::Antialiasing, false);
    QPen pen(QColor(140, 140, 140), 1.5);
    painter->setPen(pen);

    qreal cx = rect().center().x();
    qreal cy = rect().center().y();
    for (int i = -1; i <= 1; ++i) {
        qreal y = cy + i * 5;
        painter->drawLine(QPointF(cx - 8, y), QPointF(cx + 8, y));
    }
}

void ToolbarDragHandle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragStart = event->scenePos();
    m_posStart  = parentItem()->pos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void ToolbarDragHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF delta = event->scenePos() - m_dragStart;
    parentItem()->setPos(m_posStart + delta);
    event->accept();
}

void ToolbarDragHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    event->accept();
}

// ── Toolbar ─────────────────────────────────────────────────────────────────

void EditorOverlay::showToolbar()
{
    if (m_toolbarGroup) return;

    m_toolbar = new QToolBar;
    m_toolbar->setIconSize(QSize(32, 32));
    m_toolbar->setStyleSheet(
        "QToolBar { background: rgba(40,40,40,230); border: 1px solid #555; "
        "           border-radius: 6px; padding: 4px; }"
        "QToolButton { color: white; background: #444; border: 1px solid #666; "
        "              border-radius: 4px; padding: 6px 12px; margin: 2px; "
        "              font-size: 16px; }"
        "QToolButton:hover { background: #0078D4; }"
        "QToolButton:pressed { background: #005A9E; }"
    );

        // Only allow adding Buttons in the simplified editor.
        auto *actAddBtn = m_toolbar->addAction(QStringLiteral("+ Button"));
        m_toolbar->addSeparator();
        auto *actDelete = m_toolbar->addAction(QStringLiteral("Delete"));
        auto *actProps  = m_toolbar->addAction(QStringLiteral("Properties"));
        m_toolbar->addSeparator();
        auto *actPages  = m_toolbar->addAction(QStringLiteral("Page List"));
        m_toolbar->addSeparator();
        auto *actDone   = m_toolbar->addAction(QStringLiteral("Done (F2)"));

        connect(actAddBtn, &QAction::triggered, this,
            [this]() { addElement(ElementType::Button); });
        connect(actDelete, &QAction::triggered, this,
            &EditorOverlay::deleteSelected);
        connect(actProps, &QAction::triggered, this, [this]() {
        if (m_selected)
            emit editPropertiesRequested(m_selected);
        });
        connect(actPages, &QAction::triggered, this, [this]() {
        m_pageTabBar->toggle();
        });
        connect(actDone, &QAction::triggered, this, [this]() {
        QTimer::singleShot(0, this, [this]() { setEditMode(false); });
        });

    // ── Build a movable container ───────────────────────────────────────
    // Container item that holds both the drag handle and the toolbar proxy.
    // We don't use QGraphicsItemGroup because it merges child shapes;
    // instead we use a plain QGraphicsRectItem as an invisible parent.
    constexpr qreal kHandleW = 30.0;
    QSizeF tbSize = m_toolbar->sizeHint();

    // Invisible parent rect that covers drag handle + toolbar
    auto *container = new QGraphicsRectItem(0, 0,
                                            kHandleW + tbSize.width(),
                                            tbSize.height());
    container->setBrush(Qt::NoBrush);
    container->setPen(Qt::NoPen);
    container->setZValue(20000);

    // Drag handle on the left
    m_dragHandle = new ToolbarDragHandle(kHandleW, tbSize.height(), container);
    m_dragHandle->setPos(0, 0);
    m_dragHandle->setZValue(20001);

    // Toolbar proxy as child, offset to the right of the grip
    m_toolbarProxy = m_scene->addWidget(m_toolbar);
    m_toolbarProxy->setParentItem(container);
    m_toolbarProxy->setPos(kHandleW, 0);
    m_toolbarProxy->setZValue(20001);

    // Add container to scene, position at top-center
    m_scene->addItem(container);
    m_toolbarGroup = qgraphicsitem_cast<QGraphicsRectItem *>(container);

    qreal totalW = kHandleW + tbSize.width();
    container->setPos(960 - totalW / 2.0, 10);
}

void EditorOverlay::hideToolbar()
{
    if (m_toolbarGroup) {
        m_scene->removeItem(m_toolbarGroup);
        delete m_toolbarGroup;   // deletes children (drag handle + proxy + toolbar)
        m_toolbarGroup = nullptr;
        m_toolbarProxy = nullptr;
        m_toolbar      = nullptr;
        m_dragHandle   = nullptr;
    }
}

// ── Utility ─────────────────────────────────────────────────────────────────

QPointF EditorOverlay::snapToGrid(const QPointF &pos, qreal grid)
{
    return QPointF(qRound(pos.x() / grid) * grid,
                   qRound(pos.y() / grid) * grid);
}

} // namespace vt
