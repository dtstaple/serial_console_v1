#include <QApplication>

#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Serial Debug Console"));
    QApplication::setOrganizationName(QStringLiteral("Portfolio"));

    sd::MainWindow window;
    window.show();
    return app.exec();
}
