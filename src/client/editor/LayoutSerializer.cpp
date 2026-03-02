// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/LayoutSerializer.cpp

#include "LayoutSerializer.h"
#include "../layout/ButtonElement.h"
#include "../layout/LabelElement.h"
#include "../layout/PanelElement.h"

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
        pageObj[QStringLiteral("name")]     = pageName;
        pageObj[QStringLiteral("elements")] = elementsArr;
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
    case ElementType::Button: obj[QStringLiteral("type")] = QStringLiteral("button"); break;
    case ElementType::Label:  obj[QStringLiteral("type")] = QStringLiteral("label"); break;
    case ElementType::Panel:  obj[QStringLiteral("type")] = QStringLiteral("panel"); break;
    }

    obj[QStringLiteral("x")] = elem->pos().x();
    obj[QStringLiteral("y")] = elem->pos().y();
    obj[QStringLiteral("w")] = elem->elementW();
    obj[QStringLiteral("h")] = elem->elementH();
    obj[QStringLiteral("label")]        = elem->label();
    obj[QStringLiteral("bgColor")]      = elem->bgColor().name(QColor::HexArgb);
    obj[QStringLiteral("textColor")]    = elem->textColor().name(QColor::HexArgb);
    obj[QStringLiteral("fontSize")]     = elem->fontSize();
    obj[QStringLiteral("cornerRadius")] = elem->cornerRadius();

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
    qreal cornerRadius = obj[QStringLiteral("cornerRadius")].toDouble(12);

    if (type == QStringLiteral("button")) {
        auto *btn = page->addButton(id, x, y, w, h, label);
        if (!btn) return false;
        btn->setBgColor(bgColor);
        btn->setTextColor(textColor);
        btn->setFontSize(fontSize);
        btn->setCornerRadius(cornerRadius);
        if (obj.contains(QStringLiteral("activeColor")))
            btn->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
    } else if (type == QStringLiteral("label")) {
        auto *lbl = page->addLabel(id, x, y, w, h, label);
        if (!lbl) return false;
        lbl->setBgColor(bgColor);
        lbl->setTextColor(textColor);
        lbl->setFontSize(fontSize);
        lbl->setCornerRadius(cornerRadius);
        if (obj.contains(QStringLiteral("alignment")))
            lbl->setAlignment(static_cast<Qt::Alignment>(obj[QStringLiteral("alignment")].toInt()));
        if (obj.contains(QStringLiteral("drawBackground")))
            lbl->setDrawBackground(obj[QStringLiteral("drawBackground")].toBool());
    } else if (type == QStringLiteral("panel")) {
        auto *pnl = page->addPanel(id, x, y, w, h);
        if (!pnl) return false;
        pnl->setBgColor(bgColor);
        pnl->setCornerRadius(cornerRadius);
        if (obj.contains(QStringLiteral("borderColor")))
            pnl->setBorderColor(QColor(obj[QStringLiteral("borderColor")].toString()));
        if (obj.contains(QStringLiteral("borderWidth")))
            pnl->setBorderWidth(obj[QStringLiteral("borderWidth")].toDouble());
    } else {
        qWarning() << "[serializer] Unknown element type:" << type;
        return false;
    }

    return true;
}

} // namespace vt
