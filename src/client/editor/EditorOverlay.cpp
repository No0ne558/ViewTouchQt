// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/EditorOverlay.cpp

#include "EditorOverlay.h"
#include "../layout/ButtonElement.h"
#include "../layout/LabelElement.h"
#include "../layout/PanelElement.h"
#include "../layout/PageWidget.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPen>
#include <QToolButton>

namespace vt {

// ═════════════════════════════════════════════════════════════════════════════
// ResizeHandle
// ═════════════════════════════════════════════════════════════════════════════

ResizeHandle::ResizeHandle(Position pos, QGraphicsItem *parent)
    : QGraphicsRectItem(-kHandleSize / 2, -kHandleSize / 2,
                        kHandleSize, kHandleSize, parent)
    , m_pos(pos)
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

    // Update sibling handles via the overlay
    auto *overlay = qobject_cast<EditorOverlay *>(elem->scene()->parent());
    // We'll update handles at release instead, parent handles approach
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
}

EditorOverlay::~EditorOverlay()
{
    hideHandles();
    hideToolbar();
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
        qDebug() << "[editor] Edit mode OFF";
    }

    emit editModeChanged(on);
}

// ── Selection ───────────────────────────────────────────────────────────────

void EditorOverlay::selectElement(UiElement *elem)
{
    if (m_selected == elem)
        return;

    deselectAll();
    m_selected = elem;

    if (!elem)
        return;

    // Create selection rectangle
    createSelectionRect();
    createResizeHandles();
    updateHandles();

    emit selectionChanged(elem);
}

void EditorOverlay::deselectAll()
{
    hideHandles();
    m_selected = nullptr;
    emit selectionChanged(nullptr);
}

void EditorOverlay::deleteSelected()
{
    if (!m_selected || !m_engine->activePage())
        return;

    QString id = m_selected->elementId();
    deselectAll();
    m_engine->activePage()->removeElement(id);
}

// ── Add element ─────────────────────────────────────────────────────────────

void EditorOverlay::addElement(ElementType type)
{
    auto *pg = m_engine->activePage();
    if (!pg) return;

    // Generate unique ID
    QString prefix;
    switch (type) {
    case ElementType::Button: prefix = QStringLiteral("btn_"); break;
    case ElementType::Label:  prefix = QStringLiteral("lbl_"); break;
    case ElementType::Panel:  prefix = QStringLiteral("pnl_"); break;
    }

    QString id;
    do {
        id = prefix + QString::number(m_nextId++);
    } while (pg->element(id));

    // Place in center of canvas
    constexpr qreal cx = 960.0;
    constexpr qreal cy = 540.0;

    UiElement *newElem = nullptr;
    switch (type) {
    case ElementType::Button: {
        auto *btn = pg->addButton(id, cx - 100, cy - 30, 200, 60,
                                  QStringLiteral("New Button"));
        btn->setBgColor(QColor(100, 150, 200));
        btn->setTextColor(Qt::white);
        newElem = btn;
        break;
    }
    case ElementType::Label: {
        auto *lbl = pg->addLabel(id, cx - 100, cy - 20, 200, 40,
                                 QStringLiteral("New Label"));
        lbl->setTextColor(Qt::white);
        newElem = lbl;
        break;
    }
    case ElementType::Panel: {
        auto *pnl = pg->addPanel(id, cx - 100, cy - 50, 200, 100);
        pnl->setBgColor(QColor(60, 60, 60));
        newElem = pnl;
        break;
    }
    }

    if (newElem)
        selectElement(newElem);
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

        // Check if click is on the toolbar proxy
        if (m_toolbarProxy && m_toolbarProxy->contains(
                m_toolbarProxy->mapFromScene(me->scenePos()))) {
            return false;  // let toolbar process
        }

        // Check if we clicked on an element
        auto *elem = dynamic_cast<UiElement *>(item);
        if (!elem) {
            // Maybe clicked on a child item of the element
            if (item && item->parentItem())
                elem = dynamic_cast<UiElement *>(item->parentItem());
        }

        if (elem) {
            selectElement(elem);
            // Start drag
            m_dragging = true;
            m_dragStartScene = me->scenePos();
            m_dragStartPos = elem->pos();
            return true;  // consume event — don't trigger button clicks
        } else {
            deselectAll();
            return true;
        }
    }

    if (event->type() == QEvent::GraphicsSceneMouseMove && m_dragging && m_selected) {
        auto *me = static_cast<QGraphicsSceneMouseEvent *>(event);
        QPointF delta = me->scenePos() - m_dragStartScene;
        QPointF newPos = snapToGrid(m_dragStartPos + delta);
        m_selected->setPos(newPos);
        updateHandles();
        return true;
    }

    if (event->type() == QEvent::GraphicsSceneMouseRelease && m_dragging) {
        m_dragging = false;
        if (m_selected)
            updateHandles();
        return true;
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

    return false;
}

// ── Handles ─────────────────────────────────────────────────────────────────

void EditorOverlay::createSelectionRect()
{
    if (m_selectionRect) {
        delete m_selectionRect;
        m_selectionRect = nullptr;
    }

    if (!m_selected) return;

    m_selectionRect = new QGraphicsRectItem(m_selected);
    m_selectionRect->setPen(QPen(QColor(0, 120, 255), 2, Qt::DashLine));
    m_selectionRect->setBrush(Qt::NoBrush);
    m_selectionRect->setZValue(9999);
}

void EditorOverlay::createResizeHandles()
{
    qDeleteAll(m_handles);
    m_handles.clear();

    if (!m_selected) return;

    const ResizeHandle::Position positions[] = {
        ResizeHandle::TopLeft, ResizeHandle::Top, ResizeHandle::TopRight,
        ResizeHandle::Right, ResizeHandle::BottomRight, ResizeHandle::Bottom,
        ResizeHandle::BottomLeft, ResizeHandle::Left
    };

    for (auto pos : positions) {
        auto *h = new ResizeHandle(pos, m_selected);
        m_handles.append(h);
    }
}

void EditorOverlay::updateHandles()
{
    if (!m_selected) return;

    QRectF r = m_selected->boundingRect();

    if (m_selectionRect)
        m_selectionRect->setRect(r.adjusted(-3, -3, 3, 3));

    for (auto *h : m_handles)
        h->reposition(r);
}

void EditorOverlay::hideHandles()
{
    if (m_selectionRect) {
        delete m_selectionRect;
        m_selectionRect = nullptr;
    }
    qDeleteAll(m_handles);
    m_handles.clear();
}

// ── Toolbar ─────────────────────────────────────────────────────────────────

void EditorOverlay::showToolbar()
{
    if (m_toolbarProxy) return;

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

    auto *actAddBtn = m_toolbar->addAction(QStringLiteral("+ Button"));
    auto *actAddLbl = m_toolbar->addAction(QStringLiteral("+ Label"));
    auto *actAddPnl = m_toolbar->addAction(QStringLiteral("+ Panel"));
    m_toolbar->addSeparator();
    auto *actDelete = m_toolbar->addAction(QStringLiteral("Delete"));
    auto *actProps  = m_toolbar->addAction(QStringLiteral("Properties"));
    m_toolbar->addSeparator();
    auto *actDone   = m_toolbar->addAction(QStringLiteral("Done (F2)"));

    connect(actAddBtn, &QAction::triggered, this,
            [this]() { addElement(ElementType::Button); });
    connect(actAddLbl, &QAction::triggered, this,
            [this]() { addElement(ElementType::Label); });
    connect(actAddPnl, &QAction::triggered, this,
            [this]() { addElement(ElementType::Panel); });
    connect(actDelete, &QAction::triggered, this,
            &EditorOverlay::deleteSelected);
    connect(actProps, &QAction::triggered, this, [this]() {
        if (m_selected)
            emit editPropertiesRequested(m_selected);
    });
    connect(actDone, &QAction::triggered, this, [this]() {
        setEditMode(false);
    });

    m_toolbarProxy = m_scene->addWidget(m_toolbar);
    m_toolbarProxy->setZValue(20000);
    // Position at top-center of the 1920×1080 design canvas
    m_toolbarProxy->setPos(960 - m_toolbar->sizeHint().width() / 2.0, 10);
}

void EditorOverlay::hideToolbar()
{
    if (m_toolbarProxy) {
        m_scene->removeItem(m_toolbarProxy);
        delete m_toolbarProxy;
        m_toolbarProxy = nullptr;
        m_toolbar = nullptr;  // deleted with proxy
    }
}

// ── Utility ─────────────────────────────────────────────────────────────────

QPointF EditorOverlay::snapToGrid(const QPointF &pos, qreal grid)
{
    return QPointF(qRound(pos.x() / grid) * grid,
                   qRound(pos.y() / grid) * grid);
}

} // namespace vt
