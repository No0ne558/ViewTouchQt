// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/displays/DisplayConfig.h — Data model for a remote display / terminal.

#ifndef VT_DISPLAY_CONFIG_H
#define VT_DISPLAY_CONFIG_H

#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace vt {

/// Configuration for a single remote display / terminal client.
struct DisplayConfig {
    QString uuid;          // unique identifier
    QString name;          // human-readable name ("Bar 1", "Patio")
    QString ipAddress;     // display IP (e.g. "192.168.1.72")
    QString printer;       // CUPS printer name assigned to this display
    bool    active = true; // whether this display is currently active

    /// Create a new config with a fresh UUID.
    static DisplayConfig create(const QString &name, const QString &ip)
    {
        DisplayConfig cfg;
        cfg.uuid      = QUuid::createUuid().toString(QUuid::WithoutBraces);
        cfg.name      = name;
        cfg.ipAddress = ip;
        return cfg;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj[QStringLiteral("uuid")]      = uuid;
        obj[QStringLiteral("name")]      = name;
        obj[QStringLiteral("ipAddress")] = ipAddress;
        obj[QStringLiteral("printer")]   = printer;
        obj[QStringLiteral("active")]    = active;
        return obj;
    }

    static DisplayConfig fromJson(const QJsonObject &obj)
    {
        DisplayConfig cfg;
        cfg.uuid      = obj[QStringLiteral("uuid")].toString();
        cfg.name      = obj[QStringLiteral("name")].toString();
        cfg.ipAddress = obj[QStringLiteral("ipAddress")].toString();
        cfg.printer   = obj[QStringLiteral("printer")].toString();
        cfg.active    = obj[QStringLiteral("active")].toBool(true);
        return cfg;
    }
};

} // namespace vt

#endif // VT_DISPLAY_CONFIG_H
