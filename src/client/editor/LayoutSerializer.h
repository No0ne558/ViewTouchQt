// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/editor/LayoutSerializer.h — Save/load page layouts to/from JSON.

#ifndef VT_LAYOUT_SERIALIZER_H
#define VT_LAYOUT_SERIALIZER_H

#include "../layout/LayoutEngine.h"

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace vt {

/// Serializes and deserializes the entire layout (all pages + elements)
/// to/from a JSON file.
///
/// File format:
/// @code
/// {
///   "version": 1,
///   "pages": [
///     {
///       "name": "main",
///       "elements": [
///         {
///           "id": "btn_press",
///           "type": "button",
///           "x": 760, "y": 480, "w": 400, "h": 120,
///           "label": "PRESS",
///           "bgColor": "#a0a0a0",
///           "textColor": "#000000",
///           "fontSize": 32,
///           "edges": ["flat"]
///         }
///       ]
///     }
///   ]
/// }
/// @endcode
class LayoutSerializer {
public:
    /// Save the current layout to a JSON file.
    /// Returns true on success.
    static bool saveToFile(const LayoutEngine *engine, const QString &filePath);

    /// Load a layout from a JSON file, replacing the current layout.
    /// Returns true on success.
    static bool loadFromFile(LayoutEngine *engine, const QString &filePath);

    /// Serialize the engine to a JSON object (useful for in-memory operations).
    static QJsonObject serialize(const LayoutEngine *engine);

    /// Deserialize from a JSON object, replacing engine contents.
    static bool deserialize(LayoutEngine *engine, const QJsonObject &root);

private:
    static QJsonObject serializeElement(const UiElement *elem);
    static bool deserializeElement(PageWidget *page, const QJsonObject &obj);

    static constexpr int kFormatVersion = 1;
};

} // namespace vt

#endif // VT_LAYOUT_SERIALIZER_H
