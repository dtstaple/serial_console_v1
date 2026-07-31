#include "models/LogFilterProxy.h"
#include "models/LogModel.h"

namespace sd {

LogFilterProxy::LogFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent) {
    m_enabled = { LogLevel::Info, LogLevel::Warning,
                  LogLevel::Error, LogLevel::Debug };
}

void LogFilterProxy::setLevelEnabled(LogLevel level, bool enabled) {
    const bool has = m_enabled.contains(level);
    if (enabled == has) return;
    if (enabled) m_enabled.insert(level);
    else         m_enabled.remove(level);
    invalidateFilter();
}

bool LogFilterProxy::isLevelEnabled(LogLevel level) const {
    return m_enabled.contains(level);
}

void LogFilterProxy::setShowUnclassified(bool on) {
    if (m_showUnclassified == on) return;
    m_showUnclassified = on;
    invalidateFilter();
}

void LogFilterProxy::setSearchText(const QString &text) {
    if (m_search == text) return;
    m_search = text;
    invalidateFilter();
}

bool LogFilterProxy::filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const {
    Q_UNUSED(sourceParent);

    const auto *model = qobject_cast<LogModel *>(sourceModel());
    if (!model) return true;

    const LogMessage &m = model->messageAt(sourceRow);

    // Markers and system notices are always visible.
    if (m.direction != Direction::Marker && m.direction != Direction::System) {
        if (m.level == LogLevel::None) {
            if (!m_showUnclassified) return false;
        } else if (!m_enabled.contains(m.level)) {
            return false;
        }
    }

    if (!m_search.isEmpty() &&
        !m.text.contains(m_search, Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

} // namespace sd
