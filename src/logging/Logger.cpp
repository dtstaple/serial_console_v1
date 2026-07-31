#include "logging/Logger.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

namespace sd {
namespace Logger {

namespace {

QString csvEscape(const QString &field) {
    QString f = field;
    const bool needsQuote = f.contains(QLatin1Char(',')) ||
                            f.contains(QLatin1Char('"')) ||
                            f.contains(QLatin1Char('\n')) ||
                            f.contains(QLatin1Char('\r'));
    if (needsQuote) {
        f.replace(QLatin1String("\""), QLatin1String("\"\""));
        f = QLatin1Char('"') + f + QLatin1Char('"');
    }
    return f;
}

// Parses a single logical CSV line into fields, honouring quotes and the
// doubled-quote escape.
QStringList csvParseLine(const QString &line) {
    QStringList fields;
    QString cur;
    bool inQuotes = false;
    int i = 0;
    while (i < line.size()) {
        const QChar c = line.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    cur.append(QLatin1Char('"'));
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur.append(c);
            }
        } else {
            if (c == QLatin1Char('"')) {
                inQuotes = true;
            } else if (c == QLatin1Char(',')) {
                fields.append(cur);
                cur.clear();
            } else {
                cur.append(c);
            }
        }
        ++i;
    }
    fields.append(cur);
    return fields;
}

} // namespace

bool saveText(const QString &path, const QVector<LogMessage> &messages, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }
    QTextStream out(&file);
    for (const LogMessage &m : messages) {
        out << QLatin1Char('[') << m.timestamp.toString(QStringLiteral("HH:mm:ss.zzz"))
            << QLatin1String("] [") << directionToString(m.direction) << QLatin1Char(']');
        const QString lvl = levelToString(m.level);
        if (!lvl.isEmpty())
            out << QLatin1Char(' ') << lvl;
        out << QLatin1Char(' ') << m.text << QLatin1Char('\n');
    }
    return true;
}

bool saveCsv(const QString &path, const QVector<LogMessage> &messages, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }
    QTextStream out(&file);
    out << "timestamp,direction,level,message\n";
    for (const LogMessage &m : messages) {
        out << csvEscape(m.timestamp.toString(Qt::ISODateWithMs)) << ','
            << csvEscape(directionToString(m.direction))          << ','
            << csvEscape(levelToString(m.level))                  << ','
            << csvEscape(m.text)                                  << '\n';
    }
    return true;
}

QVector<LogMessage> loadCsv(const QString &path, bool *ok, QString *error) {
    QVector<LogMessage> result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        if (ok) *ok = false;
        return result;
    }

    QTextStream in(&file);
    bool firstLine = true;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (firstLine) {
            firstLine = false;
            // Skip the header if present; otherwise treat it as data.
            if (line.startsWith(QLatin1String("timestamp,")))
                continue;
        }
        if (line.isEmpty())
            continue;

        const QStringList fields = csvParseLine(line);
        if (fields.size() < 4)
            continue;

        LogMessage m;
        m.timestamp = QDateTime::fromString(fields.at(0), Qt::ISODateWithMs);
        if (!m.timestamp.isValid())
            m.timestamp = QDateTime::currentDateTime();
        m.direction = directionFromString(fields.at(1));
        m.level     = levelFromString(fields.at(2));
        m.text      = fields.at(3);
        result.append(m);
    }

    if (ok) *ok = true;
    return result;
}

} // namespace Logger
} // namespace sd
