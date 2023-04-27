SOURCES+= \
   $$PWD/broker.cpp
HEADERS+= \
   $$PWD/broker.h \
   $$PWD/brokersettings.h

include($$PWD/commands/commands.pri)
include($$PWD/events/events.pri)
include($$PWD/interceptor/interceptor.pri)
include($$PWD/replies/replies.pri)
include($$PWD/sync/sync.pri)
include($$PWD/workers/workers.pri)