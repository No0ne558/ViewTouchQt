// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/PageTabBar.cpp

#include "PageTabBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

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
    bool wasVisible = isVisible();
    if (wasVisible) {
        destroyWidget();
        buildWidget();
    }
}

void PageTabBar::setVisible(bool visible)
{
    if (visible && !m_proxy) {
        buildWidget();
    } else if (!visible && m_proxy) {
        destroyWidget();
    }
}

// ── Internal ────────────────────────────────────────────────────────────────

void PageTabBar::buildWidget()
{
    m_bar = new QWidget;
    m_bar->setStyleSheet(
        "QWidget#pageTabBar { background: rgba(25,25,25,220); "
        "                     border-top: 2px solid #0078D4; }"
        "QPushButton { color: #ccc; background: #333; border: 1px solid #555; "
        "              border-radius: 4px; padding: 8px 18px; margin: 3px; "
        "              font-size: 15px; min-width: 80px; }"
        "QPushButton:hover { background: #0078D4; color: white; }"
        "QPushButton:checked { background: #0078D4; color: white; "
        "                      border: 2px solid #40a0ff; font-weight: bold; }"
    );
    m_bar->setObjectName(QStringLiteral("pageTabBar"));

    auto *layout = new QHBoxLayout(m_bar);
    layout->setContentsMargins(10, 4, 10, 4);

    // "Pages:" label
    auto *label = new QLabel(QStringLiteral("Pages:"));
    label->setStyleSheet(QStringLiteral(
        "color: #aaa; font-size: 14px; font-weight: bold; padding-right: 6px;"));
    layout->addWidget(label);

    QStringList pages = m_engine->pageNames();
    pages.sort();
    QString activeName = m_engine->activePageName();

    m_tabs.clear();
    for (const QString &name : pages) {
        auto *btn = new QPushButton(name);
        btn->setCheckable(true);
        btn->setChecked(name == activeName);
        btn->setCursor(Qt::PointingHandCursor);

        connect(btn, &QPushButton::clicked, this, [this, name, btn]() {
            // Uncheck all, check this one
            for (auto *other : m_tabs)
                other->setChecked(false);
            btn->setChecked(true);

            m_engine->showPage(name);
            emit pageSelected(name);
        });

        layout->addWidget(btn);
        m_tabs.append(btn);
    }

    layout->addStretch();

    // Size the widget
    m_bar->adjustSize();

    // Add to scene
    m_proxy = m_scene->addWidget(m_bar);
    m_proxy->setZValue(19000);

    // Position at the bottom of the 1920×1080 canvas
    qreal barH = m_bar->sizeHint().height();
    m_proxy->setPos(0, 1080 - barH);

    // Stretch to full width
    m_bar->setFixedWidth(1920);
}

void PageTabBar::destroyWidget()
{
    if (m_proxy) {
        m_scene->removeItem(m_proxy);
        delete m_proxy;  // also deletes m_bar
        m_proxy = nullptr;
        m_bar   = nullptr;
        m_tabs.clear();
    }
}

} // namespace vt
