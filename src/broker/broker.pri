SOURCES+= \
   $$PWD/broker.cpp
HEADERS+= \
   $$PWD/broker.h

include($$PWD/sync/sync.pri)
include($$PWD/replies/replies.pri)
include($$PWD/events/events.pri)
include($$PWD/commands/commands.pri)
include($$PWD/workers/workers.pri)
include($$PWD/interceptor/interceptor.pri)