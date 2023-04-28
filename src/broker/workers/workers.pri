SOURCES+= \
   $$PWD/loggingworker.cpp \
   $$PWD/mockworker.cpp \
   $$PWD/repeaterworker.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/loggingworker.h \
   $$PWD/mockworker.h \
   $$PWD/repeaterworker.h \
   $$PWD/worker.h

include($$PWD/private/private.pri)
include($$PWD/settings/settings.pri)