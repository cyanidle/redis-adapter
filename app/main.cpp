#include <QCoreApplication>
#include <QThread>
#include "settings-parsing/filereader.h"
#include "launcher.h"

int main (int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto launcher = new Radapter::Launcher();
    launcher->init();
    launcher->onRun();
    return app.exec();
}

