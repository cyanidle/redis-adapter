file (STRINGS ".qtmodules" QTMODULES)
message("Used modules: $$QTMODULES")
QT -= gui
QT += QTMODULES
