SOURCES+= \
   $$PWD/serializablesettings.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/qmaputils.hpp \
   $$PWD/serializer.hpp \
   $$PWD/serializablesettings.h \
   $$PWD/dictreader.h

include($$PWD/adapters/settings-parsing_adapters.pri)