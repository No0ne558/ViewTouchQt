// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/LayoutEngine.cpp

#include "LayoutEngine.h"

#include <QDebug>

namespace vt {

LayoutEngine::LayoutEngine(QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , m_scene(scene)
{
}

// ── Page management ─────────────────────────────────────────────────────────

PageWidget *LayoutEngine::createPage(const QString &name)
{
    if (m_pages.contains(name)) {
        qWarning() << "[layout] Page already exists:" << name;
        return nullptr;
    }

    auto *pg = new PageWidget(name, this);
    m_pages.insert(name, pg);

    // Forward button clicks with the page name attached.
    connect(pg, &PageWidget::buttonClicked, this,
            [this, name](const QString &elementId) {
                emit buttonClicked(name, elementId);
            });

    return pg;
}

bool LayoutEngine::removePage(const QString &name)
{
    auto it = m_pages.find(name);
    if (it == m_pages.end())
        return false;

    PageWidget *pg = it.value();

    if (pg == m_activePage) {
        qWarning() << "[layout] Cannot remove active page:" << name;
        return false;
    }

    m_pages.erase(it);
    delete pg;
    return true;
}

void LayoutEngine::clearAll()
{
    if (m_activePage) {
        m_activePage->detachFromScene();
        m_activePage = nullptr;
    }
    qDeleteAll(m_pages);
    m_pages.clear();
}

PageWidget *LayoutEngine::page(const QString &name) const
{
    return m_pages.value(name, nullptr);
}

QStringList LayoutEngine::pageNames() const
{
    return m_pages.keys();
}

// ── Navigation ──────────────────────────────────────────────────────────────

bool LayoutEngine::showPage(const QString &name)
{
    PageWidget *pg = m_pages.value(name, nullptr);
    if (!pg) {
        qWarning() << "[layout] Page not found:" << name;
        return false;
    }

    if (pg == m_activePage)
        return true;  // already shown

    // Detach the old page.
    if (m_activePage)
        m_activePage->detachFromScene();

    // Attach the new page.
    m_activePage = pg;
    m_activePage->attachToScene(m_scene);

    qInfo() << "[layout] Switched to page:" << name;
    emit pageChanged(name);
    return true;
}

QString LayoutEngine::activePageName() const
{
    return m_activePage ? m_activePage->name() : QString();
}

} // namespace vt
