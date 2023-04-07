SOURCES+= \
   $$PWD/mockworker.cpp \
   $$PWD/loggingworker.cpp \
   $$PWD/worker.cpp
HEADERS+= \
   $$PWD/loggingworker.h \
   $$PWD/loggingworkersettings.h \
   $$PWD/mockworker.h \
   $$PWD/worker_field.hpp \
   $$PWD/workersettings.h \
   $$PWD/worker.h \
   $$PWD/mockworkersettings.h

include($$PWD/private/private.pri)