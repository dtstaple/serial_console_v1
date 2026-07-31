#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QVector>

#include "models/LogMessage.h"
#include "logging/Logger.h"

using namespace sd;

class LoggerTest : public QObject {
    Q_OBJECT
private slots:
    void classifyLevels();
    void csvRoundTrip();
};

void LoggerTest::classifyLevels() {
    QCOMPARE(classifyLevel(QStringLiteral("ERROR: sensor timeout")), LogLevel::Error);
    QCOMPARE(classifyLevel(QStringLiteral("WARN low battery")),       LogLevel::Warning);
    QCOMPARE(classifyLevel(QStringLiteral("[DEBUG] tick=42")),        LogLevel::Debug);
    QCOMPARE(classifyLevel(QStringLiteral("INFO boot ok")),          LogLevel::Info);
    QCOMPARE(classifyLevel(QStringLiteral("plain reading 3.14")),     LogLevel::None);
}

void LoggerTest::csvRoundTrip() {
    QVector<LogMessage> in;
    LogMessage a;
    a.timestamp = QDateTime::fromString(QStringLiteral("2026-01-01T12:00:00.123"),
                                        Qt::ISODateWithMs);
    a.direction = Direction::Rx;
    a.level     = LogLevel::Error;
    a.text      = QStringLiteral("value = \"3,14\"\nwith comma");   // exercises escaping
    in.append(a);

    QTemporaryFile file;
    QVERIFY(file.open());
    const QString path = file.fileName();
    file.close();

    QVERIFY(Logger::saveCsv(path, in));

    bool ok = false;
    QVector<LogMessage> out = Logger::loadCsv(path, &ok);
    QVERIFY(ok);
    QCOMPARE(out.size(), 1);
    QCOMPARE(out.at(0).direction, Direction::Rx);
    QCOMPARE(out.at(0).level,     LogLevel::Error);
    QCOMPARE(out.at(0).text,      a.text);
    QCOMPARE(out.at(0).timestamp, a.timestamp);
}

QTEST_MAIN(LoggerTest)
#include "test_logger.moc"
