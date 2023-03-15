SOURCES+= \
   $$PWD/broker.cpp
HEADERS+= \
   $$PWD/broker.h

include($$PWD/interceptors/broker_interceptors.pri)
include($$PWD/sync/broker_sync.pri)
include($$PWD/replies/broker_replies.pri)
include($$PWD/worker/broker_worker.pri)
include($$PWD/events/broker_events.pri)
include($$PWD/commands/broker_commands.pri)