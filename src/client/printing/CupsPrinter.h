// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/printing/CupsPrinter.h — CUPS printer discovery and test-print utility.

#ifndef VT_CUPS_PRINTER_H
#define VT_CUPS_PRINTER_H

#include <QString>
#include <QStringList>

namespace vt {

/// Light wrapper around the CUPS C API for discovering printers and sending
/// plain-text test prints suitable for 80 mm thermal receipt printers
/// (42-character line width).
class CupsPrinter {
public:
    /// Return the names of all printers known to the CUPS server.
    static QStringList availablePrinters();

    /// Send a test page to the named printer.
    /// The receipt is formatted for 42-column width and ends with a
    /// paper-cut ESC/POS command.
    /// Returns true on success.
    static bool printTestPage(const QString &printerName);

    /// Send arbitrary raw data (bytes) to a printer.
    /// Returns true on success.
    static bool printRaw(const QString &printerName, const QByteArray &data,
                         const QString &jobTitle = QStringLiteral("ViewTouchQt"));

    /// Number of characters per line on an 80 mm thermal printer.
    static constexpr int kLineWidth = 42;
};

} // namespace vt

#endif // VT_CUPS_PRINTER_H
