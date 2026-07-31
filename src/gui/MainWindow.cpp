#include "gui/MainWindow.h"

#include "gui/TerminalView.h"
#include "models/LogModel.h"
#include "models/LogFilterProxy.h"
#include "logging/Logger.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QKeySequence>
#include <QTimer>
#include <QDateTime>
#include <QFont>

namespace sd {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    m_serial = new SerialManager(this);
    m_model  = new LogModel(this);
    m_proxy  = new LogFilterProxy(this);
    m_proxy->setSourceModel(m_model);

    m_replayTimer = new QTimer(this);
    m_replayTimer->setSingleShot(true);

    buildUi();
    buildToolbar();
    buildMenus();

    connect(m_serial, &SerialManager::opened,        this, &MainWindow::onOpened);
    connect(m_serial, &SerialManager::closed,        this, &MainWindow::onClosed);
    connect(m_serial, &SerialManager::errorOccurred, this, &MainWindow::onError);
    connect(m_serial, &SerialManager::lineReceived,  this, &MainWindow::onLineReceived);
    connect(m_serial, &SerialManager::lineSent,      this, &MainWindow::onLineSent);
    connect(m_replayTimer, &QTimer::timeout,         this, &MainWindow::onReplayTick);

    refreshPorts();
    setConnectedState(false);
    setWindowTitle(tr("Serial Debug Console"));
    resize(1000, 640);
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(6, 6, 6, 6);
    outer->setSpacing(6);

    m_view = new TerminalView(central);
    m_view->setModel(m_proxy);
    setupColumns();
    outer->addWidget(m_view, 1);

    auto *inputRow = new QHBoxLayout();
    m_input = new QLineEdit(central);
    m_input->setPlaceholderText(tr("Type a command and press Enter\u2026"));
    m_input->setFont(m_view->font());
    m_sendBtn = new QPushButton(tr("Send"), central);
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(m_sendBtn);
    outer->addLayout(inputRow);

    setCentralWidget(central);

    connect(m_input,   &QLineEdit::returnPressed, this, &MainWindow::sendCurrentInput);
    connect(m_sendBtn, &QPushButton::clicked,     this, &MainWindow::sendCurrentInput);

    m_statusLabel = new QLabel(tr("Disconnected"), this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::setupColumns() {
    auto *header = m_view->horizontalHeader();
    header->setSectionResizeMode(LogModel::ColTime,      QHeaderView::ResizeToContents);
    header->setSectionResizeMode(LogModel::ColDelta,     QHeaderView::ResizeToContents);
    header->setSectionResizeMode(LogModel::ColDirection, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(LogModel::ColLevel,     QHeaderView::ResizeToContents);
    header->setSectionResizeMode(LogModel::ColMessage,   QHeaderView::Stretch);
}

void MainWindow::buildToolbar() {
    // --- Connection row ---
    auto *tb = addToolBar(tr("Connection"));
    tb->setMovable(false);

    tb->addWidget(new QLabel(tr(" Port: ")));
    m_portCombo = new QComboBox();
    m_portCombo->setMinimumWidth(160);
    tb->addWidget(m_portCombo);

    auto *refreshAct = new QAction(tr("Refresh"), this);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshPorts);
    tb->addAction(refreshAct);

    tb->addSeparator();

    tb->addWidget(new QLabel(tr(" Baud: ")));
    m_baudCombo = new QComboBox();
    for (qint32 b : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600})
        m_baudCombo->addItem(QString::number(b), b);
    m_baudCombo->setCurrentText(QStringLiteral("115200"));
    tb->addWidget(m_baudCombo);

    tb->addWidget(new QLabel(tr(" Data: ")));
    m_dataCombo = new QComboBox();
    m_dataCombo->addItem(QStringLiteral("5"), static_cast<int>(QSerialPort::Data5));
    m_dataCombo->addItem(QStringLiteral("6"), static_cast<int>(QSerialPort::Data6));
    m_dataCombo->addItem(QStringLiteral("7"), static_cast<int>(QSerialPort::Data7));
    m_dataCombo->addItem(QStringLiteral("8"), static_cast<int>(QSerialPort::Data8));
    m_dataCombo->setCurrentText(QStringLiteral("8"));
    tb->addWidget(m_dataCombo);

    tb->addWidget(new QLabel(tr(" Parity: ")));
    m_parityCombo = new QComboBox();
    m_parityCombo->addItem(tr("None"),  static_cast<int>(QSerialPort::NoParity));
    m_parityCombo->addItem(tr("Even"),  static_cast<int>(QSerialPort::EvenParity));
    m_parityCombo->addItem(tr("Odd"),   static_cast<int>(QSerialPort::OddParity));
    m_parityCombo->addItem(tr("Mark"),  static_cast<int>(QSerialPort::MarkParity));
    m_parityCombo->addItem(tr("Space"), static_cast<int>(QSerialPort::SpaceParity));
    tb->addWidget(m_parityCombo);

    tb->addWidget(new QLabel(tr(" Stop: ")));
    m_stopCombo = new QComboBox();
    m_stopCombo->addItem(QStringLiteral("1"),   static_cast<int>(QSerialPort::OneStop));
    m_stopCombo->addItem(QStringLiteral("1.5"), static_cast<int>(QSerialPort::OneAndHalfStop));
    m_stopCombo->addItem(QStringLiteral("2"),   static_cast<int>(QSerialPort::TwoStop));
    tb->addWidget(m_stopCombo);

    tb->addSeparator();

    m_connectBtn = new QPushButton(tr("Connect"));
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    tb->addWidget(m_connectBtn);

    // --- Filter row ---
    addToolBarBreak();
    auto *fb = addToolBar(tr("Filter"));
    fb->setMovable(false);
    fb->addWidget(new QLabel(tr(" Search: ")));
    m_search = new QLineEdit();
    m_search->setPlaceholderText(tr("filter messages\u2026"));
    m_search->setClearButtonEnabled(true);
    m_search->setMaximumWidth(280);
    fb->addWidget(m_search);
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_proxy->setSearchText(t);
    });
}

void MainWindow::buildMenus() {
    // File
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *saveAct = fileMenu->addAction(tr("&Save Log\u2026"));
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveLog);

    auto *openAct = fileMenu->addAction(tr("&Open Session for Replay\u2026"));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openSessionForReplay);

    fileMenu->addSeparator();
    auto *clearAct = fileMenu->addAction(tr("&Clear Terminal"));
    connect(clearAct, &QAction::triggered, this, &MainWindow::clearTerminal);

    fileMenu->addSeparator();
    auto *quitAct = fileMenu->addAction(tr("E&xit"));
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    // View / filters
    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    auto makeLevelAction = [&](const QString &name) {
        QAction *a = viewMenu->addAction(name);
        a->setCheckable(true);
        a->setChecked(true);
        connect(a, &QAction::toggled, this, &MainWindow::onLevelFilterToggled);
        return a;
    };
    viewMenu->addSection(tr("Log levels"));
    m_actInfo    = makeLevelAction(tr("Show INFO"));
    m_actWarning = makeLevelAction(tr("Show WARNING"));
    m_actError   = makeLevelAction(tr("Show ERROR"));
    m_actDebug   = makeLevelAction(tr("Show DEBUG"));
    m_actOther   = makeLevelAction(tr("Show unclassified"));

    viewMenu->addSeparator();
    m_actShowDelta = viewMenu->addAction(tr("Show time differences (\u0394t)"));
    m_actShowDelta->setCheckable(true);
    m_actShowDelta->setChecked(true);
    connect(m_actShowDelta, &QAction::toggled, this, [this](bool on) {
        m_model->setShowDelta(on);
        m_view->setColumnHidden(LogModel::ColDelta, !on);
    });

    // Session
    auto *sessionMenu = menuBar()->addMenu(tr("&Session"));
    auto *markerAct = sessionMenu->addAction(tr("Insert &Marker\u2026"));
    markerAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+M")));
    connect(markerAct, &QAction::triggered, this, &MainWindow::insertMarker);
}

SerialConfig MainWindow::currentConfig() const {
    SerialConfig cfg;
    cfg.baudRate = m_baudCombo->currentData().toInt();
    cfg.dataBits = static_cast<QSerialPort::DataBits>(m_dataCombo->currentData().toInt());
    cfg.parity   = static_cast<QSerialPort::Parity>(m_parityCombo->currentData().toInt());
    cfg.stopBits = static_cast<QSerialPort::StopBits>(m_stopCombo->currentData().toInt());
    return cfg;
}

void MainWindow::refreshPorts() {
    const QString current = m_portCombo->currentData().toString();
    m_portCombo->clear();

    const auto ports = SerialManager::availablePorts();
    for (const PortInfo &p : ports) {
        const QString label = QStringLiteral("%1  \u2014  %2").arg(p.name, p.description);
        m_portCombo->addItem(label, p.name);
    }
    if (m_portCombo->count() == 0)
        m_portCombo->addItem(tr("(no ports found)"), QString());

    const int idx = m_portCombo->findData(current);
    if (idx >= 0)
        m_portCombo->setCurrentIndex(idx);
}

void MainWindow::toggleConnection() {
    if (m_serial->isOpen()) {
        m_serial->close();
        return;
    }
    const QString port = m_portCombo->currentData().toString();
    if (port.isEmpty()) {
        QMessageBox::warning(this, tr("No port"), tr("Select a serial port first."));
        return;
    }
    m_serial->open(port, currentConfig());
}

void MainWindow::sendCurrentInput() {
    const QString text = m_input->text();
    if (text.isEmpty())
        return;
    if (!m_serial->isOpen()) {
        QMessageBox::warning(this, tr("Not connected"),
                             tr("Connect to a port before sending."));
        return;
    }
    if (m_serial->sendLine(text))
        m_input->clear();
}

void MainWindow::insertMarker() {
    bool ok = false;
    const QString label = QInputDialog::getText(
        this, tr("Insert Marker"), tr("Marker label:"),
        QLineEdit::Normal, tr("MARKER"), &ok);
    if (!ok)
        return;

    const QString bar = QString(40, QLatin1Char('-'));
    appendMessage(Direction::Marker, LogLevel::None, bar);
    appendMessage(Direction::Marker, LogLevel::None,
                  label.isEmpty() ? tr("MARKER") : label);
    appendMessage(Direction::Marker, LogLevel::None, bar);
}

void MainWindow::saveLog() {
    if (m_model->messages().isEmpty()) {
        QMessageBox::information(this, tr("Nothing to save"), tr("The terminal is empty."));
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Log"), QStringLiteral("session.csv"),
        tr("CSV files (*.csv);;Text files (*.txt)"));
    if (path.isEmpty())
        return;

    QString error;
    bool ok;
    if (path.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive))
        ok = Logger::saveText(path, m_model->messages(), &error);
    else
        ok = Logger::saveCsv(path, m_model->messages(), &error);

    if (!ok)
        QMessageBox::critical(this, tr("Save failed"), error);
    else
        statusBar()->showMessage(tr("Saved %1").arg(path), 4000);
}

void MainWindow::openSessionForReplay() {
    if (m_serial->isOpen()) {
        QMessageBox::warning(this, tr("Disconnect first"),
                             tr("Disconnect from the live port before replaying a session."));
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Session"), QString(),
        tr("CSV files (*.csv);;All files (*)"));
    if (path.isEmpty())
        return;

    bool ok = false;
    QString error;
    QVector<LogMessage> msgs = Logger::loadCsv(path, &ok, &error);
    if (!ok) {
        QMessageBox::critical(this, tr("Open failed"), error);
        return;
    }
    if (msgs.isEmpty()) {
        QMessageBox::information(this, tr("Empty session"),
                                 tr("No messages found in that file."));
        return;
    }

    m_model->clear();
    m_replayQueue = msgs;
    m_replayIndex = 0;
    appendMessage(Direction::System, LogLevel::Info,
                  tr("Replaying %1 messages from %2").arg(msgs.size()).arg(path));
    onReplayTick();
}

void MainWindow::onReplayTick() {
    if (m_replayIndex >= m_replayQueue.size()) {
        appendMessage(Direction::System, LogLevel::Info, tr("Replay finished"));
        return;
    }

    m_model->append(m_replayQueue.at(m_replayIndex));
    ++m_replayIndex;

    if (m_replayIndex >= m_replayQueue.size()) {
        appendMessage(Direction::System, LogLevel::Info, tr("Replay finished"));
        return;
    }

    // Schedule the next message using the recorded gap, capped at 2s so long
    // idle periods don't stall the playback.
    const QDateTime &cur  = m_replayQueue.at(m_replayIndex - 1).timestamp;
    const QDateTime &next = m_replayQueue.at(m_replayIndex).timestamp;
    qint64 gap = cur.msecsTo(next);
    if (gap < 0)    gap = 0;
    if (gap > 2000) gap = 2000;
    m_replayTimer->start(static_cast<int>(gap));
}

void MainWindow::clearTerminal() {
    m_model->clear();
}

void MainWindow::onOpened(const QString &portName) {
    setConnectedState(true);
    appendMessage(Direction::System, LogLevel::Info,
                  tr("Connected to %1 @ %2 baud")
                      .arg(portName).arg(m_baudCombo->currentData().toInt()));
}

void MainWindow::onClosed() {
    setConnectedState(false);
    appendMessage(Direction::System, LogLevel::Info, tr("Disconnected"));
}

void MainWindow::onError(const QString &message) {
    appendMessage(Direction::System, LogLevel::Error, message);
    statusBar()->showMessage(message, 5000);
}

void MainWindow::onLineReceived(const QString &line) {
    appendMessage(Direction::Rx, classifyLevel(line), line);
}

void MainWindow::onLineSent(const QString &line) {
    appendMessage(Direction::Tx, LogLevel::None, line);
}

void MainWindow::onLevelFilterToggled() {
    m_proxy->setLevelEnabled(LogLevel::Info,    m_actInfo->isChecked());
    m_proxy->setLevelEnabled(LogLevel::Warning, m_actWarning->isChecked());
    m_proxy->setLevelEnabled(LogLevel::Error,   m_actError->isChecked());
    m_proxy->setLevelEnabled(LogLevel::Debug,   m_actDebug->isChecked());
    m_proxy->setShowUnclassified(m_actOther->isChecked());
}

void MainWindow::setConnectedState(bool connected) {
    m_connectBtn->setText(connected ? tr("Disconnect") : tr("Connect"));
    m_portCombo->setEnabled(!connected);
    m_baudCombo->setEnabled(!connected);
    m_dataCombo->setEnabled(!connected);
    m_parityCombo->setEnabled(!connected);
    m_stopCombo->setEnabled(!connected);
    m_input->setEnabled(connected);
    m_sendBtn->setEnabled(connected);
    m_statusLabel->setText(connected ? tr("\u25CF Connected") : tr("Disconnected"));
}

void MainWindow::appendMessage(Direction dir, LogLevel level, const QString &text) {
    LogMessage m;
    m.timestamp = QDateTime::currentDateTime();
    m.direction = dir;
    m.level     = level;
    m.text      = text;
    m_model->append(m);
}

} // namespace sd
