#pragma once

#include <QVector>
#include <QString>

#include "models/LogMessage.h"

namespace sd {

// Stateless helpers for saving and restoring terminal sessions.
namespace Logger {

// Human-readable text log:
//   [HH:mm:ss.zzz] [RX] INFO  message text
bool saveText(const QString &path, const QVector<LogMessage> &messages,
              QString *error = nullptr);

// CSV with a header row:  timestamp,direction,level,message
// Timestamps are ISO-8601 with milliseconds so sessions round-trip exactly.
bool saveCsv(const QString &path, const QVector<LogMessage> &messages,
             QString *error = nullptr);

// Loads a CSV written by saveCsv() (used for session replay).
QVector<LogMessage> loadCsv(const QString &path, bool *ok = nullptr,
                            QString *error = nullptr);

} // namespace Logger
} // namespace sd
