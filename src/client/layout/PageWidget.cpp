// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PageWidget.cpp

#include "PageWidget.h"

#include <QDebug>

namespace vt {

PageWidget::PageWidget(const QString &name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
}

PageWidget::~PageWidget()
{
    detachFromScene();
    qDeleteAll(m_elements);
}

// ── Element factories ───────────────────────────────────────────────────────

ButtonElement *PageWidget::addButton(const QString &id, qreal x, qreal y,
                                     qreal w, qreal h, const QString &label)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new ButtonElement(id, x, y, w, h);
    btn->setLabel(label);
    registerElement(btn);

    connect(btn, &ButtonElement::clicked, this, &PageWidget::buttonClicked);

    return btn;
}

// LabelElement, PanelElement, PinEntryElement, KeypadButtonElement,
// ActionButtonElement and InfoLabelElement support removed — only
// ButtonElement is supported by PageWidget.


// ── Remove ──────────────────────────────────────────────────────────────────

bool PageWidget::removeElement(const QString &id)
{
    auto it = m_elements.find(id);
    if (it == m_elements.end())
        return false;

    UiElement *elem = it.value();

    if (m_currentScene)
        m_currentScene->removeItem(elem);

    m_elements.erase(it);
    delete elem;
    return true;
}

// ── Type replacement ────────────────────────────────────────────────────────

UiElement *PageWidget::replaceElementType(const QString &id, ElementType newType)
{
    auto it = m_elements.find(id);
    if (it == m_elements.end())
        return nullptr;

    UiElement *old = it.value();
    if (old->elementType() == newType)
        return old;  // nothing to do

    // Capture common properties from the old element
    const qreal  x      = old->pos().x();
    const qreal  y      = old->pos().y();
    const qreal  w      = old->elementW();
    const qreal  h      = old->elementH();
    const QString label  = old->label();
    const QColor  bg     = old->bgColor();
    const QColor  text   = old->textColor();
    const int     fsize  = old->fontSize();
    const qreal   radius = old->cornerRadius();

    // Remove old element from scene and hash (and delete it)
    if (m_currentScene)
        m_currentScene->removeItem(old);
    m_elements.erase(it);
    delete old;

    // Create new element of the desired type using existing factory methods
    UiElement *created = nullptr;
    if (newType == ElementType::Button) {
        auto *btn = addButton(id, x, y, w, h, label);
        created = btn;
    }

    if (created) {
        created->setBgColor(bg);
        created->setTextColor(text);
        created->setFontSize(fsize);
        created->setCornerRadius(radius);
    }

    return created;
}

UiElement *PageWidget::element(const QString &id) const
{
    return m_elements.value(id, nullptr);
}

// ── Scene integration ───────────────────────────────────────────────────────

void PageWidget::attachToScene(QGraphicsScene *scene)
{
    if (!scene)
        return;

    detachFromScene();
    m_currentScene = scene;

    for (UiElement *elem : m_elements)
        m_currentScene->addItem(elem);
}

void PageWidget::detachFromScene()
{
    if (!m_currentScene)
        return;

    for (UiElement *elem : m_elements)
        m_currentScene->removeItem(elem);

    m_currentScene = nullptr;
}

// ── Internal ────────────────────────────────────────────────────────────────

void PageWidget::registerElement(UiElement *elem)
{
    m_elements.insert(elem->elementId(), elem);

    // If the page is already active, add to scene immediately.
    if (m_currentScene)
        m_currentScene->addItem(elem);
}

} // namespace vt
