SOURCES+= \
   $$PWD/serializablesettings.cpp \
   $$PWD/reader.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/serializer.hpp \
   $$PWD/reader.h \
   $$PWD/serializablesettings.h \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h

include($$PWD/adapters/adapters.pri)
include($$PWD/experimental/experimental.pri)