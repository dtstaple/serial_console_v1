#pragma once

#include <QString>
#include <QDateTime>
#include <QLatin1String>
#include <QLatin1Char>

namespace sd {

// Where a message came from / what it represents.
enum class Direction {
    Rx,       // received from the device
    Tx,       // sent to the device
    Marker,   // a user-inserted event marker
    System    // an app notice (connected, error, replay, ...)
};

// Severity, either parsed from the text or set explicitly.
enum class LogLevel {
    None,     // no level detected
    Info,
    Warning,
    Error,
    Debug
};

// One line in the terminal. The GUI never stores formatted strings; it stores
// these objects and formats them on demand (see LogModel).
struct LogMessage {
    QDateTime timestamp;
    Direction direction = Direction::System;
    LogLevel  level     = LogLevel::None;
    QString   text;
};

// --- Display / serialization helpers -------------------------------------

inline QString directionToString(Direction d) {
    switch (d) {
        case Direction::Rx:     return QStringLiteral("RX");
        case Direction::Tx:     return QStringLiteral("TX");
        case Direction::Marker: return QStringLiteral("MARK");
        case Direction::System: return QStringLiteral("SYS");
    }
    return QStringLiteral("SYS");
}

inline QString levelToString(LogLevel l) {
    switch (l) {
        case LogLevel::Info:    return QStringLiteral("INFO");
        case LogLevel::Warning: return QStringLiteral("WARNING");
        case LogLevel::Error:   return QStringLiteral("ERROR");
        case LogLevel::Debug:   return QStringLiteral("DEBUG");
        case LogLevel::None:    return QString();
    }
    return QString();
}

inline Direction directionFromString(const QString &s) {
    const QString u = s.trimmed().toUpper();
    if (u == QLatin1String("RX"))   return Direction::Rx;
    if (u == QLatin1String("TX"))   return Direction::Tx;
    if (u == QLatin1String("MARK")) return Direction::Marker;
    return Direction::System;
}

inline LogLevel levelFromString(const QString &s) {
    const QString u = s.trimmed().toUpper();
    if (u == QLatin1String("INFO"))    return LogLevel::Info;
    if (u == QLatin1String("WARNING")) return LogLevel::Warning;
    if (u == QLatin1String("WARN"))    return LogLevel::Warning;
    if (u == QLatin1String("ERROR"))   return LogLevel::Error;
    if (u == QLatin1String("DEBUG"))   return LogLevel::Debug;
    return LogLevel::None;
}

// Heuristic: classify a raw serial line by scanning for common firmware tokens.
// Firmware output like "[ERROR] i2c timeout" or "WARN: low battery" is very
// common, so this gives useful colouring/filtering with zero configuration.
inline LogLevel classifyLevel(const QString &line) {
    const QString u = line.toUpper();
    if (u.contains(QLatin1String("ERROR")) || u.contains(QLatin1String("FAIL")) ||
        u.contains(QLatin1String("PANIC")) || u.contains(QLatin1String("FAULT")))
        return LogLevel::Error;
    if (u.contains(QLatin1String("WARN")))
        return LogLevel::Warning;
    if (u.contains(QLatin1String("DEBUG")) || u.contains(QLatin1String("DBG")))
        return LogLevel::Debug;
    if (u.contains(QLatin1String("INFO")))
        return LogLevel::Info;
    return LogLevel::None;
}

} // namespace sd
