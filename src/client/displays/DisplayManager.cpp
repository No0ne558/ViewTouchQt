// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/displays/DisplayManager.cpp

#include "DisplayManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>

namespace vt {

DisplayManager::DisplayManager(QObject *parent)
    : QObject(parent)
{
}

// ── CRUD ────────────────────────────────────────────────────────────────────

int DisplayManager::addDisplay(const DisplayConfig &cfg)
{
    m_displays.append(cfg);
    emit displaysChanged();
    return m_displays.size() - 1;
}

bool DisplayManager::removeDisplay(const QString &uuid)
{
    int idx = indexOfUuid(uuid);
    if (idx < 0) return false;
    m_displays.removeAt(idx);
    emit displaysChanged();
    return true;
}

DisplayConfig *DisplayManager::display(const QString &uuid)
{
    int idx = indexOfUuid(uuid);
    return idx >= 0 ? &m_displays[idx] : nullptr;
}

const DisplayConfig *DisplayManager::display(const QString &uuid) const
{
    int idx = indexOfUuid(uuid);
    return idx >= 0 ? &m_displays.at(idx) : nullptr;
}

DisplayConfig *DisplayManager::displayAt(int index)
{
    if (index < 0 || index >= m_displays.size()) return nullptr;
    return &m_displays[index];
}

const DisplayConfig *DisplayManager::displayAt(int index) const
{
    if (index < 0 || index >= m_displays.size()) return nullptr;
    return &m_displays.at(index);
}

bool DisplayManager::updateDisplay(const DisplayConfig &cfg)
{
    int idx = indexOfUuid(cfg.uuid);
    if (idx < 0) return false;
    m_displays[idx] = cfg;
    emit displaysChanged();
    return true;
}

bool DisplayManager::toggleActive(const QString &uuid)
{
    int idx = indexOfUuid(uuid);
    if (idx < 0) return false;
    m_displays[idx].active = !m_displays[idx].active;
    emit displaysChanged();
    return m_displays[idx].active;
}

// ── Persistence ─────────────────────────────────────────────────────────────

bool DisplayManager::saveToFile(const QString &filePath) const
{
    QJsonArray arr;
    for (const auto &d : m_displays)
        arr.append(d.toJson());

    QJsonObject root;
    root[QStringLiteral("version")]  = 1;
    root[QStringLiteral("displays")] = arr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[displays] Cannot write:" << filePath;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "[displays] Saved" << m_displays.size() << "display(s) to" << filePath;
    return true;
}

bool DisplayManager::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[displays] JSON parse error:" << err.errorString();
        return false;
    }

    QJsonArray arr = doc.object()[QStringLiteral("displays")].toArray();
    m_displays.clear();
    for (const QJsonValue &v : arr)
        m_displays.append(DisplayConfig::fromJson(v.toObject()));

    qDebug() << "[displays] Loaded" << m_displays.size() << "display(s)";
    emit displaysChanged();
    return true;
}

QString DisplayManager::defaultFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/displays.json");
}

// ── Internal ────────────────────────────────────────────────────────────────

int DisplayManager::indexOfUuid(const QString &uuid) const
{
    for (int i = 0; i < m_displays.size(); ++i) {
        if (m_displays.at(i).uuid == uuid)
            return i;
    }
    return -1;
}

} // namespace vt
