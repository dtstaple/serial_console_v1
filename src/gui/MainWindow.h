#pragma once

#include <QMainWindow>
#include <QVector>

#include "serial/SerialManager.h"
#include "models/LogMessage.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QAction;
class QLabel;
class QTimer;

namespace sd {

class LogModel;
class LogFilterProxy;
class TerminalView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshPorts();
    void toggleConnection();
    void sendCurrentInput();
    void insertMarker();
    void saveLog();
    void openSessionForReplay();
    void clearTerminal();

    void onOpened(const QString &portName);
    void onClosed();
    void onError(const QString &message);
    void onLineReceived(const QString &line);
    void onLineSent(const QString &line);

    void onLevelFilterToggled();
    void onReplayTick();

private:
    void buildUi();
    void buildToolbar();
    void buildMenus();
    void setupColumns();
    void setConnectedState(bool connected);
    void appendMessage(Direction dir, LogLevel level, const QString &text);
    SerialConfig currentConfig() const;

    // Core objects
    SerialManager  *m_serial = nullptr;
    LogModel       *m_model  = nullptr;
    LogFilterProxy *m_proxy  = nullptr;
    TerminalView   *m_view   = nullptr;

    // Connection controls
    QComboBox   *m_portCombo   = nullptr;
    QComboBox   *m_baudCombo   = nullptr;
    QComboBox   *m_dataCombo   = nullptr;
    QComboBox   *m_parityCombo = nullptr;
    QComboBox   *m_stopCombo   = nullptr;
    QPushButton *m_connectBtn  = nullptr;

    // Input row
    QLineEdit   *m_input   = nullptr;
    QPushButton *m_sendBtn = nullptr;

    // Search + filter + view actions
    QLineEdit *m_search       = nullptr;
    QAction   *m_actInfo      = nullptr;
    QAction   *m_actWarning   = nullptr;
    QAction   *m_actError     = nullptr;
    QAction   *m_actDebug     = nullptr;
    QAction   *m_actOther     = nullptr;
    QAction   *m_actShowDelta = nullptr;

    QLabel *m_statusLabel = nullptr;

    // Replay state
    QTimer              *m_replayTimer = nullptr;
    QVector<LogMessage>  m_replayQueue;
    int                  m_replayIndex = 0;
};

} // namespace sd
