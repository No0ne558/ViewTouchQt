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

LabelElement *PageWidget::addLabel(const QString &id, qreal x, qreal y,
                                   qreal w, qreal h, const QString &text)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *lbl = new LabelElement(id, x, y, w, h);
    lbl->setLabel(text);
    registerElement(lbl);
    return lbl;
}

PanelElement *PageWidget::addPanel(const QString &id, qreal x, qreal y,
                                   qreal w, qreal h)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *pnl = new PanelElement(id, x, y, w, h);
    registerElement(pnl);
    return pnl;
}

PinEntryElement *PageWidget::addPinEntry(const QString &id, qreal x, qreal y,
                                          qreal w, qreal h)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *pin = new PinEntryElement(id, x, y, w, h);
    registerElement(pin);
    return pin;
}

KeypadButtonElement *PageWidget::addKeypadButton(const QString &id, qreal x, qreal y,
                                                  qreal w, qreal h, const QString &label)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *kpd = new KeypadButtonElement(id, x, y, w, h);
    kpd->setLabel(label);
    registerElement(kpd);

    connect(kpd, &KeypadButtonElement::keyPressed, this, &PageWidget::keypadPressed);

    return kpd;
}

ActionButtonElement *PageWidget::addActionButton(const QString &id, qreal x, qreal y,
                                                  qreal w, qreal h, const QString &label,
                                                  ActionType action)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *act = new ActionButtonElement(id, x, y, w, h);
    act->setLabel(label);
    act->setActionType(action);
    registerElement(act);

    connect(act, &ActionButtonElement::actionTriggered, this, &PageWidget::actionTriggered);

    return act;
}

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
