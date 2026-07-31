#include "models/LogModel.h"

namespace sd {

LogModel::LogModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int LogModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_messages.size();
}

int LogModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size())
        return {};

    const LogMessage &m = m_messages.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColTime:      return m.timestamp.toString(QStringLiteral("HH:mm:ss.zzz"));
        case ColDelta:     return m_showDelta ? formatDelta(index.row()) : QString();
        case ColDirection: return directionToString(m.direction);
        case ColLevel:     return levelToString(m.level);
        case ColMessage:   return m.text;
        default:           return {};
        }
    case Qt::ForegroundRole: {
        const QColor c = colorForRow(index.row());
        if (c.isValid()) return c;
        return {};
    }
    case Qt::ToolTipRole:
        if (index.column() == ColMessage)
            return m.text;
        return {};
    default:
        return {};
    }
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return {};
    if (orientation == Qt::Horizontal) {
        switch (section) {
        case ColTime:      return QStringLiteral("Time");
        case ColDelta:     return QStringLiteral("\u0394t");   // "Δt"
        case ColDirection: return QStringLiteral("Dir");
        case ColLevel:     return QStringLiteral("Level");
        case ColMessage:   return QStringLiteral("Message");
        default:           return {};
        }
    }
    return section + 1;
}

void LogModel::append(const LogMessage &msg) {
    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append(msg);
    endInsertRows();
}

void LogModel::setMessages(const QVector<LogMessage> &msgs) {
    beginResetModel();
    m_messages = msgs;
    endResetModel();
}

void LogModel::clear() {
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

void LogModel::setShowDelta(bool on) {
    if (m_showDelta == on) return;
    m_showDelta = on;
    if (!m_messages.isEmpty())
        emit dataChanged(index(0, ColDelta),
                         index(m_messages.size() - 1, ColDelta),
                         {Qt::DisplayRole});
}

QString LogModel::formatDelta(int row) const {
    if (row <= 0) return {};
    const QDateTime &prev = m_messages.at(row - 1).timestamp;
    const QDateTime &cur  = m_messages.at(row).timestamp;
    if (!prev.isValid() || !cur.isValid()) return {};

    const qint64 ms = prev.msecsTo(cur);
    if (ms < 0) return {};
    if (ms < 1000)
        return QStringLiteral("+%1ms").arg(ms);
    const double s = ms / 1000.0;
    return QStringLiteral("+%1s").arg(QString::number(s, 'f', 2));
}

QColor LogModel::colorForRow(int row) const {
    const LogMessage &m = m_messages.at(row);

    if (m.direction == Direction::Marker)
        return QColor(0xC5, 0x92, 0xFF);   // markers stand out (violet)
    if (m.direction == Direction::System)
        return QColor(0x88, 0x88, 0x88);   // app notices, muted grey

    // Severity wins over direction so errors/warnings always pop, even on RX.
    switch (m.level) {
    case LogLevel::Error:   return QColor(0xE0, 0x3B, 0x3B);  // red
    case LogLevel::Warning: return QColor(0xC8, 0x94, 0x1C);  // amber
    case LogLevel::Debug:   return QColor(0x6C, 0x9E, 0xD8);  // muted blue
    case LogLevel::Info:    return QColor(0xB4, 0xB4, 0xB4);  // near-normal
    case LogLevel::None:    break;
    }

    // No detected level: colour by direction for quick scanning.
    if (m.direction == Direction::Tx) return QColor(0x4F, 0x8C, 0xFF);  // blue
    if (m.direction == Direction::Rx) return QColor(0x3F, 0xB9, 0x50);  // green
    return {};   // invalid -> view uses its default text colour
}

} // namespace sd
