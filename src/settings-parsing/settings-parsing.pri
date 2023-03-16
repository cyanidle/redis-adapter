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

include($$PWD/adapters/settings-parsing_adapters.pri)