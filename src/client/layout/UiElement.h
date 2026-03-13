// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/layout/UiElement.h — Base class for all UI elements in the layout engine.

#ifndef VT_UI_ELEMENT_H
#define VT_UI_ELEMENT_H

#include <QColor>
#include <QFont>
#include <QGraphicsObject>
#include <QString>
#include <QTimer>

namespace vt {

/// Element types for identification and serialization.
enum class ElementType {
    Button,
};

/// Base class for every visual element placed on a PageWidget.
///
/// Each element has:
///   - A unique string id
///   - Position (x, y) and size (w, h) in the 1920×1080 design space
///   - A label / text
///   - Background and text colours
///   - A corner radius
///
/// Subclasses override paint() to provide type-specific rendering.
class UiElement : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QString elementId READ elementId)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
public:
    explicit UiElement(const QString &id, ElementType type,
                       qreal x, qreal y, qreal w, qreal h,
                       QGraphicsItem *parent = nullptr);
    ~UiElement() override = default;

    // ── Identity ────────────────────────────────────────────────────────
    const QString &elementId() const { return m_id; }
    ElementType    elementType() const { return m_type; }

    // ── Geometry (design-space coordinates) ─────────────────────────────
    QRectF boundingRect() const override;

    qreal elementW() const { return m_rect.width(); }
    qreal elementH() const { return m_rect.height(); }

    /// Move the element to a new position in design space.
    void moveTo(qreal x, qreal y);

    /// Resize the element.
    void resizeTo(qreal w, qreal h);

    // ── Appearance ──────────────────────────────────────────────────────
    const QString &label() const { return m_label; }
    void setLabel(const QString &text);

    const QColor &bgColor() const { return m_bgColor; }
    virtual void setBgColor(const QColor &c);

    const QColor &textColor() const { return m_textColor; }
    void setTextColor(const QColor &c);

    qreal cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(qreal r);

    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);

    const QString &fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);

    bool fontBold() const { return m_fontBold; }
    void setFontBold(bool bold);

    /// Build the QFont that this element should use for painting.
    QFont buildFont() const;

    /// Return a curated list of POS-friendly font families.
    static QStringList availableFontFamilies();

    /// Whether this element is inherited when another page inherits this page.
    bool isInheritable() const { return m_inheritable; }
    void setInheritable(bool on) { m_inheritable = on; }

signals:
    void labelChanged();
    void elementMoved(qreal x, qreal y);
    void elementResized(qreal w, qreal h);

protected:
    QRectF  m_rect;
    QString m_id;
    ElementType m_type;

    QString m_label;
    QColor  m_bgColor      = QColor(160, 160, 160);
    QColor  m_textColor    = Qt::black;
    qreal   m_cornerRadius = 12.0;
    int     m_fontSize     = 24;
    QString m_fontFamily   = QStringLiteral("Sans");
    bool    m_fontBold     = true;
    bool    m_inheritable  = false;

public:
    // Button interaction behaviour. Stored on the element so it can be
    // serialized and edited via the PropertyDialog. Default is Blink.
    enum class ButtonBehavior {
        Blink = 0,
        Toggle,
        None,
        DoubleTap,
    };

    ButtonBehavior behavior() const { return m_behavior; }
    void setBehavior(ButtonBehavior b) { m_behavior = b; }

protected:
    ButtonBehavior m_behavior = ButtonBehavior::Blink;
};

} // namespace vt

#endif // VT_UI_ELEMENT_H
