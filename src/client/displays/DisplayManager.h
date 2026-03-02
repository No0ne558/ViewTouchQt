// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/displays/DisplayManager.h — Manages the list of display configurations.

#ifndef VT_DISPLAY_MANAGER_H
#define VT_DISPLAY_MANAGER_H

#include "DisplayConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QString>

namespace vt {

/// Owns the list of display configs and persists them to a JSON file.
class DisplayManager : public QObject {
    Q_OBJECT
public:
    explicit DisplayManager(QObject *parent = nullptr);

    // ── CRUD ────────────────────────────────────────────────────────────
    const QList<DisplayConfig> &displays() const { return m_displays; }
    int count() const { return m_displays.size(); }

    /// Add a new display, returns its index.
    int addDisplay(const DisplayConfig &cfg);

    /// Remove a display by UUID.  Returns true if found.
    bool removeDisplay(const QString &uuid);

    /// Find a display by UUID (returns nullptr if not found).
    DisplayConfig *display(const QString &uuid);
    const DisplayConfig *display(const QString &uuid) const;

    /// Find a display by index.
    DisplayConfig *displayAt(int index);
    const DisplayConfig *displayAt(int index) const;

    /// Update an existing display in-place.
    bool updateDisplay(const DisplayConfig &cfg);

    /// Toggle active state, returns the new state.
    bool toggleActive(const QString &uuid);

    // ── Persistence ─────────────────────────────────────────────────────
    bool saveToFile(const QString &filePath) const;
    bool loadFromFile(const QString &filePath);

    /// Default config file path.
    static QString defaultFilePath();

signals:
    void displaysChanged();

private:
    int indexOfUuid(const QString &uuid) const;
    QList<DisplayConfig> m_displays;
};

} // namespace vt

#endif // VT_DISPLAY_MANAGER_H
