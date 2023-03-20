#include <QCoreApplication>
#include <QThread>
#include "launcher.h"

int main (int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto launcher = new Radapter::Launcher();
    launcher->init();
    launcher->run();
    return app.exec();
}
