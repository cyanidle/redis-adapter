SOURCES+= \
   $$PWD/fileworker.cpp \
   $$PWD/mockworker.cpp \
   $$PWD/repeaterworker.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/repeaterworker.h \
   $$PWD/fileworker.h \
   $$PWD/mockworker.h \
   $$PWD/worker.h

include($$PWD/private/private.pri)
include($$PWD/settings/settings.pri)