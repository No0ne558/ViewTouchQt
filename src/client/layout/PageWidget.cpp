// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/PageWidget.cpp

#include "PageWidget.h"

#include <QDebug>
#include "ButtonElement.h"

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

ButtonElement *PageWidget::addImageButton(const QString &id, qreal x, qreal y,
                                           qreal w, qreal h, const QString &imagePath)
{
    if (m_elements.contains(id)) {
        qWarning() << "[layout] Duplicate element id:" << id << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new ButtonElement(id, x, y, w, h, ElementType::Image);
    btn->setLabel(QString());
    if (!imagePath.isEmpty())
        btn->setImagePath(imagePath);
    btn->setImageOnly(true);
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
    QString imagePath;
    bool imageOnly = false;
    if (old->elementType() == ElementType::Button) {
        imagePath = static_cast<ButtonElement *>(old)->imagePath();
        imageOnly = static_cast<ButtonElement *>(old)->imageOnly();
    }
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
    } else if (newType == ElementType::Image) {
        auto *img = addImageButton(id, x, y, w, h, QString());
        created = img;
    }

    if (created) {
        created->setBgColor(bg);
        created->setTextColor(text);
        created->setFontSize(fsize);
        created->setCornerRadius(radius);
        if (created->elementType() == ElementType::Button) {
            auto *cb = static_cast<ButtonElement *>(created);
            if (!imagePath.isEmpty()) cb->setImagePath(imagePath);
            if (imageOnly) cb->setImageOnly(true);
        }
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
