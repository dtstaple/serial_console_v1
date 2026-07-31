#include "serial/SerialManager.h"

#include <QSerialPortInfo>

namespace sd {

SerialManager::SerialManager(QObject *parent)
    : QObject(parent) {
    connect(&m_port, &QSerialPort::readyRead,
            this, &SerialManager::handleReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred,
            this, &SerialManager::handleError);
}

SerialManager::~SerialManager() {
    if (m_port.isOpen())
        m_port.close();
}

bool SerialManager::isOpen() const {
    return m_port.isOpen();
}

QString SerialManager::portName() const {
    return m_port.portName();
}

QList<PortInfo> SerialManager::availablePorts() {
    QList<PortInfo> result;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        PortInfo p;
        p.name        = info.portName();
        p.description = info.description().isEmpty()
                            ? QStringLiteral("(no description)")
                            : info.description();
        result.append(p);
    }
    return result;
}

bool SerialManager::open(const QString &portName, const SerialConfig &config) {
    if (m_port.isOpen())
        m_port.close();

    m_port.setPortName(portName);
    m_port.setBaudRate(config.baudRate);
    m_port.setDataBits(config.dataBits);
    m_port.setParity(config.parity);
    m_port.setStopBits(config.stopBits);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open %1: %2")
                               .arg(portName, m_port.errorString()));
        return false;
    }

    m_rxBuffer.clear();
    emit opened(portName);
    return true;
}

void SerialManager::close() {
    if (!m_port.isOpen())
        return;
    m_port.close();
    m_rxBuffer.clear();
    emit closed();
}

bool SerialManager::sendLine(const QString &text) {
    if (!m_port.isOpen()) {
        emit errorOccurred(tr("Cannot send: port is not open."));
        return false;
    }

    QString line = text;
    if (!line.endsWith(QLatin1Char('\n')))
        line.append(QLatin1Char('\n'));

    const QByteArray bytes = line.toUtf8();
    if (m_port.write(bytes) < 0) {
        emit errorOccurred(tr("Write failed: %1").arg(m_port.errorString()));
        return false;
    }

    // Echo the trimmed line (without the newline) for the terminal view.
    emit lineSent(text.trimmed());
    return true;
}

void SerialManager::handleReadyRead() {
    m_rxBuffer.append(m_port.readAll());

    // Emit each complete line; keep any trailing partial line buffered.
    int newlineIndex;
    while ((newlineIndex = m_rxBuffer.indexOf('\n')) != -1) {
        QByteArray lineBytes = m_rxBuffer.left(newlineIndex);
        m_rxBuffer.remove(0, newlineIndex + 1);

        // Strip trailing '\r' from CRLF endings common on microcontrollers.
        if (lineBytes.endsWith('\r'))
            lineBytes.chop(1);

        emit lineReceived(QString::fromUtf8(lineBytes));
    }

    // Guard: a device that never sends '\n' shouldn't grow the buffer forever.
    constexpr int kMaxBuffer = 64 * 1024;
    if (m_rxBuffer.size() > kMaxBuffer) {
        emit lineReceived(QString::fromUtf8(m_rxBuffer));
        m_rxBuffer.clear();
    }
}

void SerialManager::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError || error == QSerialPort::NotOpenError)
        return;

    // ResourceError usually means the device was physically unplugged.
    if (error == QSerialPort::ResourceError) {
        emit errorOccurred(tr("Device error: %1").arg(m_port.errorString()));
        close();
        return;
    }

    emit errorOccurred(m_port.errorString());
}

} // namespace sd
