#include <QCoreApplication>
#include <QThread>
#include "launcher.h"

int main (int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto launcher = new Radapter::Launcher();
    return launcher->exec();
}
