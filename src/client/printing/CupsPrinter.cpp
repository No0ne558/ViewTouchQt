// SPDX-License-Identifier: GPL-3.0-or-later
// src/client/printing/CupsPrinter.cpp — CUPS printer discovery and test-print.

#include "CupsPrinter.h"

#include <QDateTime>
#include <QDebug>

#include <cups/cups.h>

namespace vt {

QStringList CupsPrinter::availablePrinters()
{
    QStringList result;

    cups_dest_t *dests = nullptr;
    int numDests = cupsGetDests(&dests);

    for (int i = 0; i < numDests; ++i)
        result << QString::fromLocal8Bit(dests[i].name);

    cupsFreeDests(numDests, dests);

    qDebug() << "[cups] Found" << result.size() << "printer(s):" << result;
    return result;
}

bool CupsPrinter::printTestPage(const QString &printerName)
{
    // Build a 42-column receipt-style test page.
    QByteArray data;

    // ESC/POS: initialize printer
    data.append("\x1b\x40", 2);

    auto line = [](const QString &text) -> QByteArray {
        QString padded = text.leftJustified(kLineWidth);
        return padded.toUtf8() + "\n";
    };

    auto center = [](const QString &text) -> QByteArray {
        int pad = (kLineWidth - text.length()) / 2;
        if (pad < 0) pad = 0;
        QString padded = QString(pad, ' ') + text;
        return padded.toUtf8() + "\n";
    };

    auto separator = []() -> QByteArray {
        return QByteArray(kLineWidth, '-') + "\n";
    };

    data += center(QStringLiteral("*** VIEWTOUCHQT ***"));
    data += center(QStringLiteral("Printer Test Page"));
    data += separator();
    data += line(QStringLiteral("Printer: ") + printerName);
    data += line(QStringLiteral("Date: ") + QDateTime::currentDateTime().toString(Qt::ISODate));
    data += separator();
    data += line(QStringLiteral("Column test (42 chars):"));
    data += QByteArray("123456789012345678901234567890123456789012\n");
    data += separator();
    data += center(QStringLiteral("If you can read this,"));
    data += center(QStringLiteral("the printer is working!"));
    data += separator();
    data += "\n\n\n";

    // ESC/POS: feed and partial cut
    data.append("\x1d\x56\x42\x03", 4);   // GS V B 3  (feed 3 lines then cut)

    return printRaw(printerName, data, QStringLiteral("ViewTouchQt Test"));
}

bool CupsPrinter::printRaw(const QString &printerName, const QByteArray &data,
                            const QString &jobTitle)
{
    if (printerName.isEmpty() || data.isEmpty())
        return false;

    QByteArray printerUtf8 = printerName.toUtf8();
    QByteArray titleUtf8   = jobTitle.toUtf8();

    // Create a raw job (no filtering — we send ESC/POS directly)
    int jobId = cupsCreateJob(CUPS_HTTP_DEFAULT,
                              printerUtf8.constData(),
                              titleUtf8.constData(),
                              0, nullptr);
    if (jobId == 0) {
        qWarning() << "[cups] Failed to create print job for" << printerName
                    << "—" << cupsLastErrorString();
        return false;
    }

    http_status_t st = cupsStartDocument(CUPS_HTTP_DEFAULT,
                                         printerUtf8.constData(),
                                         jobId,
                                         titleUtf8.constData(),
                                         CUPS_FORMAT_RAW,
                                         1 /* last document */);
    if (st != HTTP_STATUS_CONTINUE) {
        qWarning() << "[cups] cupsStartDocument failed:" << cupsLastErrorString();
        cupsCancelJob(printerUtf8.constData(), jobId);
        return false;
    }

    cupsWriteRequestData(CUPS_HTTP_DEFAULT, data.constData(),
                         static_cast<size_t>(data.size()));

    cupsFinishDocument(CUPS_HTTP_DEFAULT, printerUtf8.constData());

    qInfo() << "[cups] Test page sent to" << printerName << "— job" << jobId;
    return true;
}

} // namespace vt
