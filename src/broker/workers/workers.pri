SOURCES+= \
   $$PWD/loggingworker.cpp \
   $$PWD/mockworker.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/loggingworker.h \
   $$PWD/loggingworkersettings.h \
   $$PWD/mockworker.h \
   $$PWD/mockworkersettings.h \
   $$PWD/worker.h \
   $$PWD/workersettings.h

include($$PWD/private/private.pri)