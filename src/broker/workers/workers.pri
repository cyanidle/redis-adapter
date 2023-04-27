SOURCES+= \
   $$PWD/loggingworker.cpp \
   $$PWD/mockworker.cpp \
   $$PWD/repeater.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/loggingworker.h \
   $$PWD/mockworker.h \
   $$PWD/repeater.h \
   $$PWD/worker.h

include($$PWD/private/private.pri)
include($$PWD/settings/settings.pri)