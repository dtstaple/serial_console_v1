#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QColor>

#include "models/LogMessage.h"

namespace sd {

// Stores the session's LogMessages and exposes them to a QTableView. Keeping
// the data in a model (rather than dumping formatted text into a widget) is
// what makes filtering, colouring and the Delta column clean to implement.
class LogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColTime = 0,
        ColDelta,
        ColDirection,
        ColLevel,
        ColMessage,
        ColumnCount
    };

    explicit LogModel(QObject *parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Data management
    void append(const LogMessage &msg);
    void setMessages(const QVector<LogMessage> &msgs);
    void clear();

    const QVector<LogMessage> &messages() const { return m_messages; }
    const LogMessage &messageAt(int row) const { return m_messages.at(row); }

    // Controls whether the Delta column renders "+250ms" style values.
    void setShowDelta(bool on);

private:
    QString formatDelta(int row) const;
    QColor  colorForRow(int row) const;

    QVector<LogMessage> m_messages;
    bool m_showDelta = true;
};

} // namespace sd
