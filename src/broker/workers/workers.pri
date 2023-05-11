SOURCES+= \
   $$PWD/fileworker.cpp \
   $$PWD/mockworker.cpp \
   $$PWD/processworker.cpp \
   $$PWD/pythonmoduleworker.cpp \
   $$PWD/repeaterworker.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/fileworker.h \
   $$PWD/mockworker.h \
   $$PWD/processworker.h \
   $$PWD/pythonmoduleworker.h \
   $$PWD/repeaterworker.h \
   $$PWD/worker.h

include($$PWD/private/private.pri)
include($$PWD/settings/settings.pri)