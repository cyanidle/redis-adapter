SOURCES+= \
   $$PWD/mockworker.cpp \
   $$PWD/loggingworker.cpp \
   $$PWD/worker.cpp \
   $$PWD/repeater.cpp
HEADERS+= \
   $$PWD/loggingworker.h \
   $$PWD/repeater.h \
   $$PWD/mockworker.h \
   $$PWD/worker.h

include($$PWD/private/private.pri)
include($$PWD/settings/settings.pri)