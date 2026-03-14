// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/LayoutSerializer.cpp

#include "LayoutSerializer.h"
#include "../layout/ButtonElement.h"

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
        // Only serialize Button elements; other element types are ignored
        // per the new simplified UI policy.
        for (UiElement *elem : pg->elements()) {
            if (elem->elementType() == ElementType::Button)
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
    if (elem->elementType() == ElementType::Button)
        obj[QStringLiteral("type")] = QStringLiteral("button");


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

    // Type-specific properties: only Button supported now.
    if (elem->elementType() == ElementType::Button) {
        auto *btn = static_cast<const ButtonElement *>(elem);
        obj[QStringLiteral("activeColor")] = btn->activeColor().name(QColor::HexArgb);
        // Serialize behaviour as a string for readability
        switch (elem->behavior()) {
        case UiElement::ButtonBehavior::Blink:
            obj[QStringLiteral("behavior")] = QStringLiteral("blink");
            break;
        case UiElement::ButtonBehavior::Toggle:
            obj[QStringLiteral("behavior")] = QStringLiteral("toggle");
            break;
        case UiElement::ButtonBehavior::None:
            obj[QStringLiteral("behavior")] = QStringLiteral("none");
            break;
        case UiElement::ButtonBehavior::DoubleTap:
            obj[QStringLiteral("behavior")] = QStringLiteral("doubletap");
            break;
        }
        // Serialize layer
        obj[QStringLiteral("layer")] = elem->layer();
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
        if (obj.contains(QStringLiteral("layer")))
            btn->setLayer(obj[QStringLiteral("layer")].toInt(0));
        if (obj.contains(QStringLiteral("behavior"))) {
            QString s = obj[QStringLiteral("behavior")].toString();
            if (s == QLatin1String("blink"))
                btn->setBehavior(UiElement::ButtonBehavior::Blink);
            else if (s == QLatin1String("toggle"))
                btn->setBehavior(UiElement::ButtonBehavior::Toggle);
            else if (s == QLatin1String("none"))
                btn->setBehavior(UiElement::ButtonBehavior::None);
            else if (s == QLatin1String("doubletap"))
                btn->setBehavior(UiElement::ButtonBehavior::DoubleTap);
        }
    } else {
        // Skip any non-button element types. This intentionally drops
        // elements like labels, panels, pin entries, keypads, actions, etc.
        qInfo() << "[serializer] Skipping non-button element type:" << type << "(id:" << id << ")";
        return true;
    }

    return true;
}

} // namespace vt
