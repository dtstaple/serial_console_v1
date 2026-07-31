#pragma once

#include <QTableView>

namespace sd {

// A QTableView tuned to feel like a terminal: monospaced, dense rows, and
// "follow the tail" auto-scrolling that only sticks when the user is already
// at the bottom (so scrolling up to inspect history isn't interrupted).
class TerminalView : public QTableView {
    Q_OBJECT
public:
    explicit TerminalView(QWidget *parent = nullptr);

protected:
    void rowsInserted(const QModelIndex &parent, int start, int end) override;

private:
    bool isAtBottom() const;
};

} // namespace sd
