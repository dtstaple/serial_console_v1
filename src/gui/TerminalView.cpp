#include "gui/TerminalView.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QFontDatabase>

namespace sd {

TerminalView::TerminalView(QWidget *parent)
    : QTableView(parent) {
    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWordWrap(false);
    setAlternatingRowColors(true);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Dense, monospaced, systems-tool look.
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setFont(mono);

    verticalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 4);

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setHighlightSections(false);
}

bool TerminalView::isAtBottom() const {
    const QScrollBar *bar = verticalScrollBar();
    return bar->value() >= bar->maximum() - 2;
}

void TerminalView::rowsInserted(const QModelIndex &parent, int start, int end) {
    const bool follow = isAtBottom();
    QTableView::rowsInserted(parent, start, end);
    if (follow)
        scrollToBottom();
}

} // namespace sd
