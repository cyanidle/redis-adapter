contains(QT, gui) {
    INCLUDEPATH += \
        $$PWD
    DEFINES+=RADAPTER_GUI
    HEADERS +=

    SOURCES +=

    QT += widgets
}

FORMS +=

HEADERS += \
    $$PWD/gui/guisettings.h
