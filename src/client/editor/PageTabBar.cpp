// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageTabBar.cpp

#include "PageTabBar.h"
#include "EditorOverlay.h"   // for ToolbarDragHandle

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace vt {

PageTabBar::PageTabBar(LayoutEngine *engine, QGraphicsScene *scene,
                       QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_scene(scene)
{
}

PageTabBar::~PageTabBar()
{
    destroyWidget();
}

// ── Public API ──────────────────────────────────────────────────────────────

void PageTabBar::refresh()
{
    if (isVisible()) {
        // Remember position
        QPointF pos = m_container->pos();
        destroyWidget();
        buildWidget();
        m_container->setPos(pos);
    }
}

void PageTabBar::setVisible(bool visible)
{
    if (visible && !m_container) {
        buildWidget();
    } else if (!visible && m_container) {
        destroyWidget();
    }
}

void PageTabBar::toggle()
{
    if (isVisible()) {
        destroyWidget();
    } else {
        buildWidget();
    }
}

// ── Internal ────────────────────────────────────────────────────────────────

void PageTabBar::buildWidget()
{
    // ── Build the Qt widget ─────────────────────────────────────────────
    m_panel = new QWidget;
    m_panel->setObjectName(QStringLiteral("pageListPanel"));
    m_panel->setStyleSheet(
        "QWidget#pageListPanel { background: rgba(30,30,30,235); "
        "    border: 1px solid #555; border-radius: 8px; }"
        "QLabel#pageListTitle { color: #ddd; font-size: 15px; "
        "    font-weight: bold; padding: 4px 0; }"
        "QPushButton { color: #ccc; background: #3a3a3a; border: 1px solid #555; "
        "    border-radius: 4px; padding: 7px 14px; margin: 2px 0; "
        "    font-size: 14px; text-align: left; }"
        "QPushButton:hover { background: #0078D4; color: white; }"
        "QPushButton:checked { background: #0078D4; color: white; "
        "    border: 2px solid #40a0ff; font-weight: bold; }"
        "QPushButton#manageBtn { color: #aaa; background: transparent; "
        "    border: 1px dashed #666; font-size: 12px; margin-top: 6px; }"
        "QPushButton#manageBtn:hover { color: white; border-color: #0078D4; }"
    );

    auto *layout = new QVBoxLayout(m_panel);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(2);

    auto *title = new QLabel(QStringLiteral("Pages"));
    title->setObjectName(QStringLiteral("pageListTitle"));
    layout->addWidget(title);

    QStringList pages = m_engine->pageNames();
    pages.sort();
    QString activeName = m_engine->activePageName();

    m_tabs.clear();
    for (const QString &name : pages) {
        auto *pg = m_engine->page(name);
        int count = pg ? pg->elements().size() : 0;
        QString label = QStringLiteral("%1  (%2)").arg(name).arg(count);

        auto *btn = new QPushButton(label);
        btn->setCheckable(true);
        btn->setChecked(name == activeName);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumWidth(180);

        connect(btn, &QPushButton::clicked, this, [this, name, btn]() {
            for (auto *other : m_tabs)
                other->setChecked(false);
            btn->setChecked(true);
            m_engine->showPage(name);
            emit pageSelected(name);
        });

        layout->addWidget(btn);
        m_tabs.append(btn);
    }

    // "Manage Pages..." button at the bottom
    auto *manageBtn = new QPushButton(QStringLiteral("Manage Pages..."));
    manageBtn->setObjectName(QStringLiteral("manageBtn"));
    manageBtn->setCursor(Qt::PointingHandCursor);
    // Defer emission so the button finishes its click handling before
    // the slot chain can destroy the widget tree (prevents crash).
    connect(manageBtn, &QPushButton::clicked, this, [this]() {
        QTimer::singleShot(0, this, [this]() { emit manageRequested(); });
    });
    layout->addWidget(manageBtn);

    m_panel->adjustSize();

    // ── Build the scene container (drag handle + proxy) ─────────────────
    constexpr qreal kGripH = 24.0;
    QSizeF panelSize = m_panel->sizeHint();

    m_container = new QGraphicsRectItem(0, 0, panelSize.width(), kGripH + panelSize.height());
    m_container->setBrush(Qt::NoBrush);
    m_container->setPen(Qt::NoPen);
    m_container->setZValue(19000);

    // Drag grip at the top
    m_grip = new ToolbarDragHandle(panelSize.width(), kGripH, m_container);
    m_grip->setPos(0, 0);
    m_grip->setZValue(19001);

    // Proxy widget below the grip
    m_proxy = m_scene->addWidget(m_panel);
    m_proxy->setParentItem(m_container);
    m_proxy->setPos(0, kGripH);
    m_proxy->setZValue(19001);

    m_scene->addItem(m_container);

    // Position: right side of the canvas, under the toolbar
    m_container->setPos(1920 - panelSize.width() - 20, 70);
}

void PageTabBar::destroyWidget()
{
    if (m_container) {
        m_scene->removeItem(m_container);
        delete m_container;  // deletes children (grip + proxy + panel)
        m_container = nullptr;
        m_grip      = nullptr;
        m_proxy     = nullptr;
        m_panel     = nullptr;
        m_tabs.clear();
    }
}

} // namespace vt
