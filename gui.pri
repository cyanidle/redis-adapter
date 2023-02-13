contains(QT, gui) {
    INCLUDEPATH += \
        $$PWD/gui

    DISTFILES += \
        $$PWD/gui/workersgraph.qml

    HEADERS += \
        $$PWD/gui/mainwindow.h

    SOURCES += \
        $$PWD/gui/mainwindow.cpp

    QT += widgets gui declarative qmltypes
}
