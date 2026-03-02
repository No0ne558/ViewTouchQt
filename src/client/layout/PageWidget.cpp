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

InfoLabelElement *PageWidget::addInfoLabel(const QString &id, qreal x, qreal y,
                                            qreal w, qreal h)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *info = new InfoLabelElement(id, x, y, w, h);
    registerElement(info);
    return info;
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
    switch (newType) {
    case ElementType::Button: {
        auto *btn = addButton(id, x, y, w, h, label);
        created = btn;
        break;
    }
    case ElementType::Label: {
        auto *lbl = addLabel(id, x, y, w, h, label);
        created = lbl;
        break;
    }
    case ElementType::Panel: {
        auto *pnl = addPanel(id, x, y, w, h);
        pnl->setLabel(label);
        created = pnl;
        break;
    }
    case ElementType::PinEntry: {
        auto *pin = addPinEntry(id, x, y, w, h);
        pin->setLabel(label);
        created = pin;
        break;
    }
    case ElementType::KeypadButton: {
        auto *kpd = addKeypadButton(id, x, y, w, h, label);
        created = kpd;
        break;
    }
    case ElementType::ActionButton: {
        auto *act = addActionButton(id, x, y, w, h, label, ActionType::Login);
        created = act;
        break;
    }
    case ElementType::InfoLabel: {
        auto *info = addInfoLabel(id, x, y, w, h);
        // Label is auto-generated (version info); don't overwrite it.
        created = info;
        break;
    }
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
