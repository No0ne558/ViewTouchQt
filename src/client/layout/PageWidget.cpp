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
    QString useId = id;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] Duplicate element id:" << useId << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new ButtonElement(useId, x, y, w, h);
    btn->setLabel(label);
    registerElement(btn);

    connect(btn, &ButtonElement::clicked, this, &PageWidget::buttonClicked);

    return btn;
}

ButtonElement *PageWidget::addImageButton(const QString &id, qreal x, qreal y,
                                           qreal w, qreal h, const QString &imagePath)
{
    QString useId = id;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] Duplicate element id:" << useId << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new ButtonElement(useId, x, y, w, h, ElementType::Image);
    btn->setLabel(QString());
    if (!imagePath.isEmpty())
        btn->setImagePath(imagePath);
    btn->setImageOnly(true);
    registerElement(btn);

    connect(btn, &ButtonElement::clicked, this, &PageWidget::buttonClicked);

    return btn;
}

ButtonElement *PageWidget::addLoginButton(const QString &id, qreal x, qreal y,
                                          qreal w, qreal h, const QString &label)
{
    QString useId = id;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] Duplicate element id:" << useId << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new ButtonElement(useId, x, y, w, h, ElementType::LoginButton);
    btn->setLabel(label);
    registerElement(btn);

    connect(btn, &ButtonElement::clicked, this, &PageWidget::buttonClicked);

    return btn;
}

LoginEntryField *PageWidget::addLoginEntryField(const QString &id, qreal x, qreal y,
                                                qreal w, qreal h, int maxLength)
{
    QString useId = id;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] Duplicate element id:" << useId << "on page" << m_name;
        return nullptr;
    }

    auto *fld = new LoginEntryField(useId, x, y, w, h);
    fld->setMaxLength(maxLength);
    registerElement(fld);

    // If no default input is set for this page, make the first created
    // LoginEntryField the default input element so physical keyboard
    // events and virtual keys are routed here automatically.
    if (m_defaultInputId.isEmpty())
        m_defaultInputId = id;

    // Forward submission from field as a page-level signal
    connect(fld, &LoginEntryField::submitted, this,
            [this, id](const QString &value) { emit loginFieldSubmitted(id, value); });

    qDebug() << "[PageWidget] addLoginEntryField id=" << id << "maxLength=" << maxLength;

    return fld;
}

KeyboardButton *PageWidget::addKeyboardButton(const QString &id, qreal x, qreal y,
                                              qreal w, qreal h, const QString &assignedKey)
{
    QString useId = id;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] Duplicate element id:" << useId << "on page" << m_name;
        return nullptr;
    }

    auto *btn = new KeyboardButton(useId, x, y, w, h, assignedKey);
    registerElement(btn);

    // Keyboard buttons are local-only by default: route virtual keys to the
    // currently focused LoginEntryField instead of forwarding as a normal
    // page buttonClicked that may be sent to the server.
    connect(btn, &KeyboardButton::virtualKeyPressed, this, &PageWidget::onVirtualKeyPressed);

    qDebug() << "[PageWidget] addKeyboardButton id=" << id << "assignedKey=" << assignedKey;

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
    QString assignedKey;
    int oldMaxLength = 0;
    if (old->elementType() == ElementType::Button) {
        imagePath = static_cast<ButtonElement *>(old)->imagePath();
        imageOnly = static_cast<ButtonElement *>(old)->imageOnly();
    } else if (old->elementType() == ElementType::KeyboardButton) {
        assignedKey = static_cast<KeyboardButton *>(old)->assignedKey();
    } else if (old->elementType() == ElementType::LoginEntry) {
        oldMaxLength = static_cast<LoginEntryField *>(old)->maxLength();
    }
    const QColor  bg     = old->bgColor();
    const QColor  text   = old->textColor();
    const int     fsize  = old->fontSize();
    const UiElement::EdgeStyle style = old->edgeStyle();

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
    } else if (newType == ElementType::LoginButton) {
        auto *lbtn = addLoginButton(id, x, y, w, h, label);
        created = lbtn;
    } else if (newType == ElementType::Image) {
        auto *img = addImageButton(id, x, y, w, h, QString());
        created = img;
    } else if (newType == ElementType::LoginEntry) {
        auto *fld = addLoginEntryField(id, x, y, w, h, oldMaxLength > 0 ? oldMaxLength : 9);
        created = fld;
    } else if (newType == ElementType::KeyboardButton) {
        // Use the label as a fallback assignedKey if none captured
        QString keyArg = !assignedKey.isEmpty() ? assignedKey : label;
        auto *kb = addKeyboardButton(id, x, y, w, h, keyArg);
        created = kb;
    }

    if (created) {
        created->setBgColor(bg);
        created->setTextColor(text);
        created->setFontSize(fsize);
        created->setEdgeStyle(style);
        if (created->elementType() == ElementType::Button) {
            auto *cb = static_cast<ButtonElement *>(created);
            if (!imagePath.isEmpty()) cb->setImagePath(imagePath);
            if (imageOnly) cb->setImageOnly(true);
        } else if (created->elementType() == ElementType::KeyboardButton) {
            auto *kb = static_cast<KeyboardButton *>(created);
            if (!assignedKey.isEmpty()) kb->setAssignedKey(assignedKey);
        } else if (created->elementType() == ElementType::LoginEntry) {
            auto *fld = static_cast<LoginEntryField *>(created);
            if (oldMaxLength > 0) fld->setMaxLength(oldMaxLength);
        }
    }

    return created;
}

UiElement *PageWidget::element(const QString &id) const
{
    return m_elements.value(id, nullptr);
}

UiElement *PageWidget::cloneElement(const QString &srcId, const QString &newId)
{
    UiElement *src = element(srcId);
    if (!src) return nullptr;

    // Prepare properties to copy
    const qreal  x      = src->pos().x();
    const qreal  y      = src->pos().y();
    const qreal  w      = src->elementW();
    const qreal  h      = src->elementH();
    const QString label  = src->label();
    const QColor  bg     = src->bgColor();
    const QColor  text   = src->textColor();
    const int     fsize  = src->fontSize();
    const QString fFamily = src->fontFamily();
    const bool    fBold  = src->fontBold();
    const UiElement::EdgeStyle style = src->edgeStyle();
    const bool    inheritable = src->isInheritable();
    const int     layer = src->layer();
    const UiElement::ButtonBehavior behavior = src->behavior();

    QString useId = newId;
    if (useId.isEmpty()) {
        QString prefix = QStringLiteral("btn_");
        do { useId = prefix + QString::number(m_autoIdCounter++); } while (m_elements.contains(useId));
    } else if (m_elements.contains(useId)) {
        qWarning() << "[layout] cloneElement: duplicate id" << useId << "on page" << m_name;
        return nullptr;
    }

    UiElement *created = nullptr;
    if (src->elementType() == ElementType::Button) {
        created = addButton(useId, x, y, w, h, label);
    } else if (src->elementType() == ElementType::Image) {
        auto *sbtn = static_cast<ButtonElement *>(src);
        created = addImageButton(useId, x, y, w, h, sbtn->imagePath());
    } else if (src->elementType() == ElementType::LoginEntry) {
        auto *sf = static_cast<LoginEntryField *>(src);
        created = addLoginEntryField(useId, x, y, w, h, sf->maxLength());
    } else if (src->elementType() == ElementType::KeyboardButton) {
        auto *sk = static_cast<KeyboardButton *>(src);
        created = addKeyboardButton(useId, x, y, w, h, sk->assignedKey());
    }

    if (!created) return nullptr;

    // Apply shared visual properties
    created->setBgColor(bg);
    created->setTextColor(text);
    created->setFontSize(fsize);
    created->setFontFamily(fFamily);
    created->setFontBold(fBold);
    created->setEdgeStyle(style);
    created->setInheritable(inheritable);
    created->setLayer(layer);
    created->setBehavior(behavior);

    // Type-specific copies
    if (created->elementType() == ElementType::Button) {
        auto *bnew = static_cast<ButtonElement *>(created);
        auto *bsrc = static_cast<ButtonElement *>(src);
        if (!bsrc->imagePath().isEmpty()) {
            bnew->setImagePath(bsrc->imagePath());
            bnew->setImageOnly(bsrc->imageOnly());
        }
        bnew->setActiveColor(bsrc->activeColor());
    } else if (created->elementType() == ElementType::KeyboardButton) {
        auto *knew = static_cast<KeyboardButton *>(created);
        auto *ksrc = static_cast<KeyboardButton *>(src);
        knew->setAssignedKey(ksrc->assignedKey());
    } else if (created->elementType() == ElementType::LoginEntry) {
        auto *fnew = static_cast<LoginEntryField *>(created);
        auto *fsrc = static_cast<LoginEntryField *>(src);
        fnew->setMaxLength(fsrc->maxLength());
    }

    qDebug() << "[PageWidget] cloneElement" << srcId << "->" << created->elementId();
    return created;
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

    qDebug() << "[PageWidget] registerElement id=" << elem->elementId()
             << "type=" << static_cast<int>(elem->elementType());
}

LoginEntryField *PageWidget::defaultInputField() const
{
    // If a default id is explicitly set, prefer that element.
    if (!m_defaultInputId.isEmpty()) {
        UiElement *e = m_elements.value(m_defaultInputId, nullptr);
        if (e) {
            auto *f = dynamic_cast<LoginEntryField *>(e);
            if (f)
                return f;
        }
    }

    // Otherwise return the first LoginEntryField on the page
    for (UiElement *elem : m_elements) {
        auto *f = dynamic_cast<LoginEntryField *>(elem);
        if (f)
            return f;
    }
    return nullptr;
}

void PageWidget::onVirtualKeyPressed(const QString &key)
{
    if (!m_currentScene)
        return;

    // First try the currently focused item (if any) and see if it is
    // a LoginEntryField; otherwise fall back to the page's default
    // LoginEntryField (if configured or present).
    LoginEntryField *field = nullptr;
    QGraphicsItem *focused = m_currentScene->focusItem();
    if (focused) {
        QGraphicsItem *it = focused;
        while (it) {
            UiElement *owner = dynamic_cast<UiElement *>(it);
            if (owner) {
                field = dynamic_cast<LoginEntryField *>(owner);
                if (field)
                    break;
            }
            it = it->parentItem();
        }
    }

    if (!field)
        field = defaultInputField();

    if (!field)
        return;

    if (key.compare("Backspace", Qt::CaseInsensitive) == 0 ||
        key.compare("Delete", Qt::CaseInsensitive) == 0 ||
        key.compare("DEL", Qt::CaseInsensitive) == 0) {
        field->backspace();
        return;
    }

    if (key.compare("Clear", Qt::CaseInsensitive) == 0) {
        field->clear();
        return;
    }

    if (key.compare("Enter", Qt::CaseInsensitive) == 0 ||
        key.compare("Return", Qt::CaseInsensitive) == 0) {
        emit loginFieldSubmitted(field->elementId(), field->value());
        return;
    }

    // Default: treat as single-character append (digits expected)
    if (!key.isEmpty())
        field->appendKey(key);
}

} // namespace vt
