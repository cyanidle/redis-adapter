SOURCES+= \
   $$PWD/broker.cpp
HEADERS+= \
   $$PWD/broker.h

include($$PWD/interceptors/interceptors.pri)
include($$PWD/sync/sync.pri)
include($$PWD/replies/replies.pri)
include($$PWD/worker/worker.pri)
include($$PWD/events/events.pri)
include($$PWD/commands/commands.pri)