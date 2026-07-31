#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>

#include "models/LogMessage.h"

namespace sd {

// Filters the LogModel by enabled log levels and an optional text search.
// Markers and system notices are always shown, regardless of level filters.
class LogFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit LogFilterProxy(QObject *parent = nullptr);

    void setLevelEnabled(LogLevel level, bool enabled);
    bool isLevelEnabled(LogLevel level) const;

    // Whether lines with no detected level are shown.
    void setShowUnclassified(bool on);
    bool showUnclassified() const { return m_showUnclassified; }

    void setSearchText(const QString &text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QSet<LogLevel> m_enabled;          // real levels currently shown
    bool           m_showUnclassified = true;
    QString        m_search;
};

} // namespace sd
