// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/LayoutSerializer.cpp

#include "LayoutSerializer.h"
#include "../layout/ButtonElement.h"
#include "../layout/LabelElement.h"
#include "../layout/PanelElement.h"
#include "../layout/PinEntryElement.h"
#include "../layout/KeypadButtonElement.h"
#include "../layout/ActionButtonElement.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

namespace vt {

// ── Save ────────────────────────────────────────────────────────────────────

bool LayoutSerializer::saveToFile(const LayoutEngine *engine, const QString &filePath)
{
    QJsonObject root = serialize(engine);
    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[serializer] Cannot open for writing:" << filePath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "[serializer] Layout saved to" << filePath;
    return true;
}

// ── Load ────────────────────────────────────────────────────────────────────

bool LayoutSerializer::loadFromFile(LayoutEngine *engine, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[serializer] Cannot open for reading:" << filePath;
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[serializer] JSON parse error:" << err.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "[serializer] Root is not JSON object";
        return false;
    }

    return deserialize(engine, doc.object());
}

// ── Serialize ───────────────────────────────────────────────────────────────

QJsonObject LayoutSerializer::serialize(const LayoutEngine *engine)
{
    QJsonArray pagesArr;

    for (const QString &pageName : engine->pageNames()) {
        PageWidget *pg = engine->page(pageName);
        if (!pg) continue;

        QJsonArray elementsArr;
        for (UiElement *elem : pg->elements()) {
            elementsArr.append(serializeElement(elem));
        }

        QJsonObject pageObj;
        pageObj[QStringLiteral("name")]       = pageName;
        pageObj[QStringLiteral("systemPage")] = pg->isSystemPage();
        if (!pg->inheritFrom().isEmpty())
            pageObj[QStringLiteral("inheritFrom")] = pg->inheritFrom();
        pageObj[QStringLiteral("elements")]   = elementsArr;
        pagesArr.append(pageObj);
    }

    QJsonObject root;
    root[QStringLiteral("version")] = kFormatVersion;
    root[QStringLiteral("pages")]   = pagesArr;
    return root;
}

QJsonObject LayoutSerializer::serializeElement(const UiElement *elem)
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = elem->elementId();

    switch (elem->elementType()) {
    case ElementType::Button:       obj[QStringLiteral("type")] = QStringLiteral("button"); break;
    case ElementType::Label:        obj[QStringLiteral("type")] = QStringLiteral("label"); break;
    case ElementType::Panel:        obj[QStringLiteral("type")] = QStringLiteral("panel"); break;
    case ElementType::PinEntry:     obj[QStringLiteral("type")] = QStringLiteral("pinEntry"); break;
    case ElementType::KeypadButton: obj[QStringLiteral("type")] = QStringLiteral("keypadButton"); break;
    case ElementType::ActionButton: obj[QStringLiteral("type")] = QStringLiteral("actionButton"); break;
    }

    obj[QStringLiteral("x")] = elem->pos().x();
    obj[QStringLiteral("y")] = elem->pos().y();
    obj[QStringLiteral("w")] = elem->elementW();
    obj[QStringLiteral("h")] = elem->elementH();
    obj[QStringLiteral("label")]        = elem->label();
    obj[QStringLiteral("bgColor")]      = elem->bgColor().name(QColor::HexArgb);
    obj[QStringLiteral("textColor")]    = elem->textColor().name(QColor::HexArgb);
    obj[QStringLiteral("fontSize")]     = elem->fontSize();
    obj[QStringLiteral("fontFamily")]   = elem->fontFamily();
    obj[QStringLiteral("fontBold")]     = elem->fontBold();
    obj[QStringLiteral("cornerRadius")] = elem->cornerRadius();

    if (elem->isInheritable())
        obj[QStringLiteral("inheritable")] = true;

    // Type-specific properties
    if (elem->elementType() == ElementType::Label) {
        auto *lbl = static_cast<const LabelElement *>(elem);
        obj[QStringLiteral("alignment")]      = static_cast<int>(lbl->alignment());
        obj[QStringLiteral("drawBackground")] = lbl->drawBackground();
    }
    if (elem->elementType() == ElementType::Panel) {
        auto *pnl = static_cast<const PanelElement *>(elem);
        obj[QStringLiteral("borderColor")] = pnl->borderColor().name(QColor::HexArgb);
        obj[QStringLiteral("borderWidth")] = pnl->borderWidth();
    }
    if (elem->elementType() == ElementType::Button) {
        auto *btn = static_cast<const ButtonElement *>(elem);
        obj[QStringLiteral("activeColor")] = btn->activeColor().name(QColor::HexArgb);
    }
    if (elem->elementType() == ElementType::PinEntry) {
        auto *pin = static_cast<const PinEntryElement *>(elem);
        obj[QStringLiteral("masked")]    = pin->masked();
        obj[QStringLiteral("maxLength")] = pin->maxLength();
    }
    if (elem->elementType() == ElementType::KeypadButton) {
        auto *kpd = static_cast<const KeypadButtonElement *>(elem);
        obj[QStringLiteral("keyValue")]    = kpd->keyValue();
        obj[QStringLiteral("activeColor")] = kpd->activeColor().name(QColor::HexArgb);
    }
    if (elem->elementType() == ElementType::ActionButton) {
        auto *act = static_cast<const ActionButtonElement *>(elem);
        obj[QStringLiteral("actionType")]  = static_cast<int>(act->actionType());
        obj[QStringLiteral("activeColor")] = act->activeColor().name(QColor::HexArgb);
        if (act->actionType() == ActionType::Navigation && !act->targetPage().isEmpty())
            obj[QStringLiteral("targetPage")] = act->targetPage();
    }

    return obj;
}

// ── Deserialize ─────────────────────────────────────────────────────────────

bool LayoutSerializer::deserialize(LayoutEngine *engine, const QJsonObject &root)
{
    int version = root[QStringLiteral("version")].toInt(0);
    if (version != kFormatVersion) {
        qWarning() << "[serializer] Unsupported format version:" << version;
        return false;
    }

    // Remember active page name
    QString activeName = engine->activePageName();

    // Remove all existing pages
    engine->clearAll();

    // Load pages
    QJsonArray pagesArr = root[QStringLiteral("pages")].toArray();
    QString firstPageName;

    for (const QJsonValue &pageVal : pagesArr) {
        QJsonObject pageObj = pageVal.toObject();
        QString pageName = pageObj[QStringLiteral("name")].toString();
        if (pageName.isEmpty()) continue;

        if (firstPageName.isEmpty())
            firstPageName = pageName;

        PageWidget *pg = engine->createPage(pageName);
        if (!pg) continue;

        // Restore system page flag
        pg->setSystemPage(pageObj[QStringLiteral("systemPage")].toBool(false));

        // Restore inherit-from
        if (pageObj.contains(QStringLiteral("inheritFrom")))
            pg->setInheritFrom(pageObj[QStringLiteral("inheritFrom")].toString());

        QJsonArray elemArr = pageObj[QStringLiteral("elements")].toArray();
        for (const QJsonValue &elemVal : elemArr) {
            deserializeElement(pg, elemVal.toObject());
        }
    }

    // Show the previously active page, or the first one
    if (!activeName.isEmpty() && engine->page(activeName))
        engine->showPage(activeName);
    else if (!firstPageName.isEmpty())
        engine->showPage(firstPageName);

    qDebug() << "[serializer] Layout loaded:" << pagesArr.size() << "pages";
    return true;
}

bool LayoutSerializer::deserializeElement(PageWidget *page, const QJsonObject &obj)
{
    QString id   = obj[QStringLiteral("id")].toString();
    QString type = obj[QStringLiteral("type")].toString();
    if (id.isEmpty() || type.isEmpty())
        return false;

    qreal x = obj[QStringLiteral("x")].toDouble();
    qreal y = obj[QStringLiteral("y")].toDouble();
    qreal w = obj[QStringLiteral("w")].toDouble(200);
    qreal h = obj[QStringLiteral("h")].toDouble(60);
    QString label     = obj[QStringLiteral("label")].toString();
    QColor bgColor    = QColor(obj[QStringLiteral("bgColor")].toString(QStringLiteral("#a0a0a0")));
    QColor textColor  = QColor(obj[QStringLiteral("textColor")].toString(QStringLiteral("#000000")));
    int fontSize      = obj[QStringLiteral("fontSize")].toInt(24);
    QString fontFamily = obj[QStringLiteral("fontFamily")].toString(QStringLiteral("Sans"));
    bool fontBold      = obj[QStringLiteral("fontBold")].toBool(true);
    qreal cornerRadius = obj[QStringLiteral("cornerRadius")].toDouble(12);
    bool inheritable   = obj[QStringLiteral("inheritable")].toBool(false);

    if (type == QStringLiteral("button")) {
        auto *btn = page->addButton(id, x, y, w, h, label);
        if (!btn) return false;
        btn->setBgColor(bgColor);
        btn->setTextColor(textColor);
        btn->setFontSize(fontSize);
        btn->setFontFamily(fontFamily);
        btn->setFontBold(fontBold);
        btn->setCornerRadius(cornerRadius);
        btn->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("activeColor")))
            btn->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
    } else if (type == QStringLiteral("label")) {
        auto *lbl = page->addLabel(id, x, y, w, h, label);
        if (!lbl) return false;
        lbl->setBgColor(bgColor);
        lbl->setTextColor(textColor);
        lbl->setFontSize(fontSize);
        lbl->setFontFamily(fontFamily);
        lbl->setFontBold(fontBold);
        lbl->setCornerRadius(cornerRadius);
        lbl->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("alignment")))
            lbl->setAlignment(static_cast<Qt::Alignment>(obj[QStringLiteral("alignment")].toInt()));
        if (obj.contains(QStringLiteral("drawBackground")))
            lbl->setDrawBackground(obj[QStringLiteral("drawBackground")].toBool());
    } else if (type == QStringLiteral("panel")) {
        auto *pnl = page->addPanel(id, x, y, w, h);
        if (!pnl) return false;
        pnl->setBgColor(bgColor);
        pnl->setCornerRadius(cornerRadius);
        pnl->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("borderColor")))
            pnl->setBorderColor(QColor(obj[QStringLiteral("borderColor")].toString()));
        if (obj.contains(QStringLiteral("borderWidth")))
            pnl->setBorderWidth(obj[QStringLiteral("borderWidth")].toDouble());
    } else if (type == QStringLiteral("pinEntry")) {
        auto *pin = page->addPinEntry(id, x, y, w, h);
        if (!pin) return false;
        pin->setBgColor(bgColor);
        pin->setTextColor(textColor);
        pin->setFontSize(fontSize);
        pin->setFontFamily(fontFamily);
        pin->setFontBold(fontBold);
        pin->setCornerRadius(cornerRadius);
        pin->setInheritable(inheritable);
        pin->setLabel(label);  // placeholder text
        if (obj.contains(QStringLiteral("masked")))
            pin->setMasked(obj[QStringLiteral("masked")].toBool(true));
        if (obj.contains(QStringLiteral("maxLength")))
            pin->setMaxLength(obj[QStringLiteral("maxLength")].toInt(8));
    } else if (type == QStringLiteral("keypadButton")) {
        auto *kpd = page->addKeypadButton(id, x, y, w, h, label);
        if (!kpd) return false;
        kpd->setBgColor(bgColor);
        kpd->setTextColor(textColor);
        kpd->setFontSize(fontSize);
        kpd->setFontFamily(fontFamily);
        kpd->setFontBold(fontBold);
        kpd->setCornerRadius(cornerRadius);
        kpd->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("keyValue")))
            kpd->setKeyValue(obj[QStringLiteral("keyValue")].toString());
        if (obj.contains(QStringLiteral("activeColor")))
            kpd->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
    } else if (type == QStringLiteral("actionButton")) {
        auto *act = page->addActionButton(id, x, y, w, h, label,
                        static_cast<ActionType>(obj[QStringLiteral("actionType")].toInt(0)));
        if (!act) return false;
        act->setBgColor(bgColor);
        act->setTextColor(textColor);
        act->setFontSize(fontSize);
        act->setFontFamily(fontFamily);
        act->setFontBold(fontBold);
        act->setCornerRadius(cornerRadius);
        act->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("targetPage")))
            act->setTargetPage(obj[QStringLiteral("targetPage")].toString());
        if (obj.contains(QStringLiteral("activeColor")))
            act->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
    } else {
        qWarning() << "[serializer] Unknown element type:" << type;
        return false;
    }

    return true;
}

} // namespace vt
