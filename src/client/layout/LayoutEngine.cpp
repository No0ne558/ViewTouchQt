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

bool LayoutEngine::renamePage(const QString &oldName, const QString &newName)
{
    if (oldName == newName)
        return true;

    if (!m_pages.contains(oldName)) {
        qWarning() << "[layout] Page not found for rename:" << oldName;
        return false;
    }
    if (m_pages.contains(newName)) {
        qWarning() << "[layout] Target name already exists:" << newName;
        return false;
    }

    PageWidget *pg = m_pages.take(oldName);
    pg->setName(newName);
    m_pages.insert(newName, pg);

    // Re-connect forwarding signals with the new name
    disconnect(pg, &PageWidget::buttonClicked, this, nullptr);
    connect(pg, &PageWidget::buttonClicked, this,
            [this, newName](const QString &elementId) {
                emit buttonClicked(newName, elementId);
            });

    if (m_activePage == pg)
        emit pageChanged(newName);

    qInfo() << "[layout] Renamed page" << oldName << "->" << newName;
    return true;
}

void LayoutEngine::clearAll()
{
    detachInheritedElements();
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

QStringList LayoutEngine::systemPageNames() const
{
    QStringList result;
    for (auto it = m_pages.cbegin(); it != m_pages.cend(); ++it) {
        if (it.value()->isSystemPage())
            result.append(it.key());
    }
    return result;
}

QStringList LayoutEngine::userPageNames() const
{
    QStringList result;
    for (auto it = m_pages.cbegin(); it != m_pages.cend(); ++it) {
        if (!it.value()->isSystemPage())
            result.append(it.key());
    }
    return result;
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

    // Detach inherited elements from previous page.
    detachInheritedElements();

    // Detach the old page.
    if (m_activePage)
        m_activePage->detachFromScene();

    // Attach the new page.
    m_activePage = pg;
    m_activePage->attachToScene(m_scene);

    // Attach inherited elements (if this page inherits from another).
    attachInheritedElements();

    qInfo() << "[layout] Switched to page:" << name;
    emit pageChanged(name);
    return true;
}

QString LayoutEngine::activePageName() const
{
    return m_activePage ? m_activePage->name() : QString();
}

// ── Inheritance ─────────────────────────────────────────────────────────────

void LayoutEngine::attachInheritedElements()
{
    if (!m_activePage || m_activePage->inheritFrom().isEmpty())
        return;

    PageWidget *parent = m_pages.value(m_activePage->inheritFrom(), nullptr);
    if (!parent)
        return;

    for (UiElement *elem : parent->elements()) {
        if (elem->isInheritable() && elem->scene() != m_scene) {
            m_scene->addItem(elem);
            m_inheritedItems.append(elem);
        }
    }
}

void LayoutEngine::detachInheritedElements()
{
    for (UiElement *elem : m_inheritedItems) {
        if (elem->scene() == m_scene)
            m_scene->removeItem(elem);
    }
    m_inheritedItems.clear();
}

} // namespace vt
