contains(QT, gui) {
    INCLUDEPATH += \
        $$PWD
    DEFINES+=RADAPTER_GUI
    HEADERS += \
        $$PWD/gui/mainwindow.h

    SOURCES += \
        $$PWD/gui/mainwindow.cpp

    QT += widgets
}

FORMS += \
    $$PWD/gui/mainwindow.ui

HEADERS += \
    $$PWD/gui/guisettings.h
