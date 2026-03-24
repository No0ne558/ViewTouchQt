// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/LayoutSerializer.cpp

#include "LayoutSerializer.h"
#include "../layout/ButtonElement.h"
#include "../layout/KeyboardButton.h"
#include "../layout/LoginEntryField.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>

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
            // Serialize button-like and supported element types
            if (elem->elementType() == ElementType::Button ||
                elem->elementType() == ElementType::Image ||
                elem->elementType() == ElementType::LoginEntry ||
                elem->elementType() == ElementType::KeyboardButton)
            {
                elementsArr.append(serializeElement(elem));
            }
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
    else if (elem->elementType() == ElementType::Image)
        obj[QStringLiteral("type")] = QStringLiteral("image");
    else if (elem->elementType() == ElementType::LoginEntry)
        obj[QStringLiteral("type")] = QStringLiteral("login");
    else if (elem->elementType() == ElementType::KeyboardButton)
        obj[QStringLiteral("type")] = QStringLiteral("keyboard");
    else if (elem->elementType() == ElementType::LoginButton)
        obj[QStringLiteral("type")] = QStringLiteral("loginbutton");


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
    // Serialize edge/style as a human-readable token inside an array.
    QJsonArray edgesArr;
    switch (elem->edgeStyle()) {
    case UiElement::EdgeStyle::Flat:
        edgesArr.append(QStringLiteral("flat"));
        break;
    case UiElement::EdgeStyle::Raised:
        edgesArr.append(QStringLiteral("raised"));
        break;
    case UiElement::EdgeStyle::Raised2:
        edgesArr.append(QStringLiteral("raised2"));
        break;
    case UiElement::EdgeStyle::Raised3:
        edgesArr.append(QStringLiteral("raised3"));
        break;
    case UiElement::EdgeStyle::Inset:
        edgesArr.append(QStringLiteral("inset"));
        break;
    case UiElement::EdgeStyle::Inset2:
        edgesArr.append(QStringLiteral("inset2"));
        break;
    case UiElement::EdgeStyle::Inset3:
        edgesArr.append(QStringLiteral("inset3"));
        break;
    case UiElement::EdgeStyle::Double:
        edgesArr.append(QStringLiteral("double"));
        break;
    case UiElement::EdgeStyle::Border:
        edgesArr.append(QStringLiteral("border"));
        break;
    case UiElement::EdgeStyle::Outline:
        edgesArr.append(QStringLiteral("outline"));
        break;
    case UiElement::EdgeStyle::Rounded:
        edgesArr.append(QStringLiteral("rounded"));
        break;
    case UiElement::EdgeStyle::None:
        edgesArr.append(QStringLiteral("none"));
        break;
    }
    obj[QStringLiteral("edges")] = edgesArr;

    // Shadow intensity: min/med/max
    switch (elem->shadowIntensity()) {
    case UiElement::ShadowIntensity::Min:
        obj[QStringLiteral("shadowIntensity")] = QStringLiteral("min");
        break;
    case UiElement::ShadowIntensity::Med:
        obj[QStringLiteral("shadowIntensity")] = QStringLiteral("med");
        break;
    case UiElement::ShadowIntensity::Max:
        obj[QStringLiteral("shadowIntensity")] = QStringLiteral("max");
        break;
    case UiElement::ShadowIntensity::None:
        obj[QStringLiteral("shadowIntensity")] = QStringLiteral("none");
        break;
    }

    if (elem->isInheritable())
        obj[QStringLiteral("inheritable")] = true;

    // Type-specific properties: only Button supported now.
    if (elem->elementType() == ElementType::Button) {
        auto *btn = static_cast<const ButtonElement *>(elem);
        if (!btn->imagePath().isEmpty()) {
            QString img = btn->imagePath();
            // If the image is under the system image directory, store only the filename
            QFileInfo fi(img);
            if (fi.isAbsolute()) {
                QByteArray env = qgetenv("VIEWTOUCH_IMG_DIR");
                QString sysDir = env.isEmpty() ? QStringLiteral("/opt/viewtouch/img") : QString::fromUtf8(env);
                QDir sys(sysDir);
                QString sysPath = QDir(sys.absolutePath()).absolutePath();
                if (img == sysPath || img.startsWith(sysPath + QDir::separator()))
                    img = fi.fileName();
            }
            obj[QStringLiteral("imagePath")] = img;
        }
        obj[QStringLiteral("imageOnly")] = btn->imageOnly();
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
        case UiElement::ButtonBehavior::PassThrough:
            obj[QStringLiteral("behavior")] = QStringLiteral("passthrough");
            break;
        case UiElement::ButtonBehavior::DoubleTap:
            obj[QStringLiteral("behavior")] = QStringLiteral("doubletap");
            break;
        }
        // Serialize layer
        obj[QStringLiteral("layer")] = elem->layer();
        // Behaviour stored above includes PassThrough option
    }

    // Keyboard Button: include assigned key
    if (elem->elementType() == ElementType::KeyboardButton) {
        auto *kb = static_cast<const KeyboardButton *>(elem);
        obj[QStringLiteral("assignedKey")] = kb->assignedKey();
    }

    // Login Entry: include max length
    if (elem->elementType() == ElementType::LoginEntry) {
        auto *fld = static_cast<const LoginEntryField *>(elem);
        obj[QStringLiteral("maxLength")] = fld->maxLength();
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
    // New format uses "edges" array; fall back to legacy cornerRadius when absent.
    QString edgeToken;
    QString shadowToken;
    if (obj.contains(QStringLiteral("edges"))) {
        QJsonArray earr = obj[QStringLiteral("edges")].toArray();
        if (!earr.isEmpty()) edgeToken = earr.at(0).toString();
    }
    if (obj.contains(QStringLiteral("shadowIntensity")))
        shadowToken = obj[QStringLiteral("shadowIntensity")].toString();
    bool inheritable   = obj[QStringLiteral("inheritable")].toBool(false);

    if (type == QStringLiteral("button")) {
        auto *btn = page->addButton(id, x, y, w, h, label);
        if (!btn) return false;
        btn->setBgColor(bgColor);
        btn->setTextColor(textColor);
        btn->setFontSize(fontSize);
        btn->setFontFamily(fontFamily);
        btn->setFontBold(fontBold);
        if (!edgeToken.isEmpty()) {
            if (edgeToken == QLatin1String("flat"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Flat);
            else if (edgeToken == QLatin1String("raised"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Raised);
            else if (edgeToken == QLatin1String("raised2"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Raised2);
            else if (edgeToken == QLatin1String("raised3"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Raised3);
            else if (edgeToken == QLatin1String("inset"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Inset);
            else if (edgeToken == QLatin1String("inset2"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Inset2);
            else if (edgeToken == QLatin1String("inset3"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Inset3);
            else if (edgeToken == QLatin1String("double"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Double);
            else if (edgeToken == QLatin1String("border"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Border);
            else if (edgeToken == QLatin1String("outline"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Outline);
            else if (edgeToken == QLatin1String("rounded"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Rounded);
            else
                btn->setEdgeStyle(UiElement::EdgeStyle::Flat);

            // Shadow intensity (optional)
            if (!shadowToken.isEmpty()) {
                if (shadowToken == QLatin1String("none"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::None);
                else if (shadowToken == QLatin1String("min"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Min);
                else if (shadowToken == QLatin1String("med"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
                else if (shadowToken == QLatin1String("max"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Max);
                else
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
            } else {
                btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
            }
        } else {
            // Legacy behaviour: map cornerRadius > 0 to Rounded, else Flat
            btn->setEdgeStyle(cornerRadius > 0 ? UiElement::EdgeStyle::Rounded : UiElement::EdgeStyle::Flat);
        }
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
            else if (s == QLatin1String("passthrough"))
                btn->setBehavior(UiElement::ButtonBehavior::PassThrough);
            else if (s == QLatin1String("doubletap"))
                btn->setBehavior(UiElement::ButtonBehavior::DoubleTap);
        }
        // Ensure runtime mouse acceptance matches behaviour when loaded
        btn->setAcceptedMouseButtons(btn->behavior() == UiElement::ButtonBehavior::PassThrough ? Qt::NoButton : Qt::LeftButton);
        if (obj.contains(QStringLiteral("imagePath")))
            btn->setImagePath(obj[QStringLiteral("imagePath")].toString());
        if (obj.contains(QStringLiteral("imageOnly")))
            btn->setImageOnly(obj[QStringLiteral("imageOnly")].toBool(false));
    } else if (type == QStringLiteral("image")) {
        // Image button: create image-typed element
        auto *img = page->addImageButton(id, x, y, w, h, QString());
        if (!img) return false;
        img->setBgColor(bgColor);
        img->setTextColor(textColor);
        img->setFontSize(fontSize);
        img->setFontFamily(fontFamily);
        img->setFontBold(fontBold);
        if (!edgeToken.isEmpty()) {
            if (edgeToken == QLatin1String("flat"))
                img->setEdgeStyle(UiElement::EdgeStyle::Flat);
            else if (edgeToken == QLatin1String("raised"))
                img->setEdgeStyle(UiElement::EdgeStyle::Raised);
            else if (edgeToken == QLatin1String("inset"))
                img->setEdgeStyle(UiElement::EdgeStyle::Inset);
            else if (edgeToken == QLatin1String("outline"))
                img->setEdgeStyle(UiElement::EdgeStyle::Outline);
            else if (edgeToken == QLatin1String("rounded"))
                img->setEdgeStyle(UiElement::EdgeStyle::Rounded);
            else
                img->setEdgeStyle(UiElement::EdgeStyle::Flat);

            if (!shadowToken.isEmpty()) {
                if (shadowToken == QLatin1String("none"))
                    img->setShadowIntensity(UiElement::ShadowIntensity::None);
                else if (shadowToken == QLatin1String("min"))
                    img->setShadowIntensity(UiElement::ShadowIntensity::Min);
                else if (shadowToken == QLatin1String("med"))
                    img->setShadowIntensity(UiElement::ShadowIntensity::Med);
                else if (shadowToken == QLatin1String("max"))
                    img->setShadowIntensity(UiElement::ShadowIntensity::Max);
                else
                    img->setShadowIntensity(UiElement::ShadowIntensity::Med);
            } else {
                img->setShadowIntensity(UiElement::ShadowIntensity::Med);
            }
        } else {
            img->setEdgeStyle(cornerRadius > 0 ? UiElement::EdgeStyle::Rounded : UiElement::EdgeStyle::Flat);
        }
        img->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("activeColor")))
            img->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
        if (obj.contains(QStringLiteral("layer")))
            img->setLayer(obj[QStringLiteral("layer")].toInt(0));
        if (obj.contains(QStringLiteral("behavior"))) {
            QString s = obj[QStringLiteral("behavior")].toString();
            if (s == QLatin1String("blink"))
                img->setBehavior(UiElement::ButtonBehavior::Blink);
            else if (s == QLatin1String("toggle"))
                img->setBehavior(UiElement::ButtonBehavior::Toggle);
            else if (s == QLatin1String("none"))
                img->setBehavior(UiElement::ButtonBehavior::None);
            else if (s == QLatin1String("passthrough"))
                img->setBehavior(UiElement::ButtonBehavior::PassThrough);
            else if (s == QLatin1String("doubletap"))
                img->setBehavior(UiElement::ButtonBehavior::DoubleTap);
        }
        // Ensure runtime mouse acceptance matches behaviour when loaded
        img->setAcceptedMouseButtons(img->behavior() == UiElement::ButtonBehavior::PassThrough ? Qt::NoButton : Qt::LeftButton);
        if (obj.contains(QStringLiteral("imagePath")))
            img->setImagePath(obj[QStringLiteral("imagePath")].toString());
        if (obj.contains(QStringLiteral("imageOnly")))
            img->setImageOnly(obj[QStringLiteral("imageOnly")].toBool(false));
    } else if (type == QStringLiteral("login")) {
        int maxLen = obj.contains(QStringLiteral("maxLength")) ? obj[QStringLiteral("maxLength")].toInt(9) : 9;
        auto *fld = page->addLoginEntryField(id, x, y, w, h, maxLen);
        if (!fld) return false;
        fld->setBgColor(bgColor);
        fld->setTextColor(textColor);
        fld->setFontSize(fontSize);
        fld->setFontFamily(fontFamily);
        fld->setFontBold(fontBold);
        if (!edgeToken.isEmpty()) {
            if (edgeToken == QLatin1String("flat"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Flat);
            else if (edgeToken == QLatin1String("raised"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Raised);
            else if (edgeToken == QLatin1String("raised2"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Raised2);
            else if (edgeToken == QLatin1String("raised3"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Raised3);
            else if (edgeToken == QLatin1String("inset"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Inset);
            else if (edgeToken == QLatin1String("inset2"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Inset2);
            else if (edgeToken == QLatin1String("inset3"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Inset3);
            else if (edgeToken == QLatin1String("double"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Double);
            else if (edgeToken == QLatin1String("border"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Border);
            else if (edgeToken == QLatin1String("outline"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Outline);
            else if (edgeToken == QLatin1String("rounded"))
                fld->setEdgeStyle(UiElement::EdgeStyle::Rounded);
            else
                fld->setEdgeStyle(UiElement::EdgeStyle::Flat);

            if (!shadowToken.isEmpty()) {
                if (shadowToken == QLatin1String("none"))
                    fld->setShadowIntensity(UiElement::ShadowIntensity::None);
                else if (shadowToken == QLatin1String("min"))
                    fld->setShadowIntensity(UiElement::ShadowIntensity::Min);
                else if (shadowToken == QLatin1String("med"))
                    fld->setShadowIntensity(UiElement::ShadowIntensity::Med);
                else if (shadowToken == QLatin1String("max"))
                    fld->setShadowIntensity(UiElement::ShadowIntensity::Max);
                else
                    fld->setShadowIntensity(UiElement::ShadowIntensity::Med);
            } else {
                fld->setShadowIntensity(UiElement::ShadowIntensity::Med);
            }
        } else {
            fld->setEdgeStyle(cornerRadius > 0 ? UiElement::EdgeStyle::Rounded : UiElement::EdgeStyle::Flat);
        }
        fld->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("layer")))
            fld->setLayer(obj[QStringLiteral("layer")].toInt(0));
    } else if (type == QStringLiteral("loginbutton")) {
        auto *btn = page->addLoginButton(id, x, y, w, h, label);
        if (!btn) return false;
        btn->setBgColor(bgColor);
        btn->setTextColor(textColor);
        btn->setFontSize(fontSize);
        btn->setFontFamily(fontFamily);
        btn->setFontBold(fontBold);
        if (!edgeToken.isEmpty()) {
            if (edgeToken == QLatin1String("flat"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Flat);
            else if (edgeToken == QLatin1String("raised"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Raised);
            else if (edgeToken == QLatin1String("inset"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Inset);
            else if (edgeToken == QLatin1String("outline"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Outline);
            else if (edgeToken == QLatin1String("rounded"))
                btn->setEdgeStyle(UiElement::EdgeStyle::Rounded);
            else
                btn->setEdgeStyle(UiElement::EdgeStyle::Flat);

            if (!shadowToken.isEmpty()) {
                if (shadowToken == QLatin1String("none"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::None);
                else if (shadowToken == QLatin1String("min"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Min);
                else if (shadowToken == QLatin1String("med"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
                else if (shadowToken == QLatin1String("max"))
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Max);
                else
                    btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
            } else {
                btn->setShadowIntensity(UiElement::ShadowIntensity::Med);
            }
        } else {
            btn->setEdgeStyle(cornerRadius > 0 ? UiElement::EdgeStyle::Rounded : UiElement::EdgeStyle::Flat);
        }
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
            else if (s == QLatin1String("passthrough"))
                btn->setBehavior(UiElement::ButtonBehavior::PassThrough);
            else if (s == QLatin1String("doubletap"))
                btn->setBehavior(UiElement::ButtonBehavior::DoubleTap);
        }
        // Ensure runtime mouse acceptance matches behaviour when loaded
        btn->setAcceptedMouseButtons(btn->behavior() == UiElement::ButtonBehavior::PassThrough ? Qt::NoButton : Qt::LeftButton);
    } else if (type == QStringLiteral("keyboard")) {
        QString assigned = obj.contains(QStringLiteral("assignedKey")) ? obj[QStringLiteral("assignedKey")].toString(label) : label;
        auto *kb = page->addKeyboardButton(id, x, y, w, h, assigned);
        if (!kb) return false;
        kb->setBgColor(bgColor);
        kb->setTextColor(textColor);
        kb->setFontSize(fontSize);
        kb->setFontFamily(fontFamily);
        kb->setFontBold(fontBold);
        if (!edgeToken.isEmpty()) {
            if (edgeToken == QLatin1String("flat"))
                kb->setEdgeStyle(UiElement::EdgeStyle::Flat);
            else if (edgeToken == QLatin1String("raised"))
                kb->setEdgeStyle(UiElement::EdgeStyle::Raised);
            else if (edgeToken == QLatin1String("inset"))
                kb->setEdgeStyle(UiElement::EdgeStyle::Inset);
            else if (edgeToken == QLatin1String("outline"))
                kb->setEdgeStyle(UiElement::EdgeStyle::Outline);
            else if (edgeToken == QLatin1String("rounded"))
                kb->setEdgeStyle(UiElement::EdgeStyle::Rounded);
            else
                kb->setEdgeStyle(UiElement::EdgeStyle::Flat);

            if (!shadowToken.isEmpty()) {
                if (shadowToken == QLatin1String("none"))
                    kb->setShadowIntensity(UiElement::ShadowIntensity::None);
                else if (shadowToken == QLatin1String("min"))
                    kb->setShadowIntensity(UiElement::ShadowIntensity::Min);
                else if (shadowToken == QLatin1String("med"))
                    kb->setShadowIntensity(UiElement::ShadowIntensity::Med);
                else if (shadowToken == QLatin1String("max"))
                    kb->setShadowIntensity(UiElement::ShadowIntensity::Max);
                else
                    kb->setShadowIntensity(UiElement::ShadowIntensity::Med);
            } else {
                kb->setShadowIntensity(UiElement::ShadowIntensity::Med);
            }
        } else {
            kb->setEdgeStyle(cornerRadius > 0 ? UiElement::EdgeStyle::Rounded : UiElement::EdgeStyle::Flat);
        }
        kb->setInheritable(inheritable);
        if (obj.contains(QStringLiteral("activeColor")))
            kb->setActiveColor(QColor(obj[QStringLiteral("activeColor")].toString()));
        if (obj.contains(QStringLiteral("layer")))
            kb->setLayer(obj[QStringLiteral("layer")].toInt(0));
        if (obj.contains(QStringLiteral("behavior"))) {
            QString s = obj[QStringLiteral("behavior")].toString();
            if (s == QLatin1String("blink"))
                kb->setBehavior(UiElement::ButtonBehavior::Blink);
            else if (s == QLatin1String("toggle"))
                kb->setBehavior(UiElement::ButtonBehavior::Toggle);
            else if (s == QLatin1String("none"))
                kb->setBehavior(UiElement::ButtonBehavior::None);
            else if (s == QLatin1String("passthrough"))
                kb->setBehavior(UiElement::ButtonBehavior::PassThrough);
            else if (s == QLatin1String("doubletap"))
                kb->setBehavior(UiElement::ButtonBehavior::DoubleTap);
        }
        kb->setAcceptedMouseButtons(kb->behavior() == UiElement::ButtonBehavior::PassThrough ? Qt::NoButton : Qt::LeftButton);
        if (obj.contains(QStringLiteral("assignedKey")))
            kb->setAssignedKey(obj[QStringLiteral("assignedKey")].toString());
    } else {
        // Skip any non-button element types. This intentionally drops
        // elements like labels, panels, pin entries, keypads, actions, etc.
        qInfo() << "[serializer] Skipping non-button element type:" << type << "(id:" << id << ")";
        return true;
    }


    return true;
}

} // namespace vt
