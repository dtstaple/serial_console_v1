#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QString>
#include <QList>

namespace sd {

// Snapshot of a connectable port for the UI.
struct PortInfo {
    QString name;         // e.g. "COM3" or "/dev/ttyUSB0"
    QString description;  // human-readable
};

// Serial line configuration.
struct SerialConfig {
    qint32                baudRate = 115200;
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity   parity   = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
};

// Owns the QSerialPort and turns the raw byte stream into complete text lines.
//
// This class contains NO GUI code and talks to the rest of the app purely
// through signals/slots. QSerialPort is asynchronous (it emits readyRead()
// on the event loop and never blocks), so the UI stays responsive without a
// worker thread. Because the class is self-contained, it can also be moved to
// a QThread with moveToThread() if you want to demonstrate that pattern --
// see the README for details.
class SerialManager : public QObject {
    Q_OBJECT
public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager() override;

    bool    isOpen() const;
    QString portName() const;

    static QList<PortInfo> availablePorts();

public slots:
    bool open(const QString &portName, const SerialConfig &config);
    void close();
    // Sends text to the device; a trailing '\n' is appended if not present.
    bool sendLine(const QString &text);

signals:
    void lineReceived(const QString &line);  // one complete RX line (no newline)
    void lineSent(const QString &line);      // echo of a TX line
    void opened(const QString &portName);
    void closed();
    void errorOccurred(const QString &message);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort m_port;
    QByteArray  m_rxBuffer;   // accumulates partial lines between reads
};

} // namespace sd
