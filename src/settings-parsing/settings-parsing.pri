SOURCES+= \
   $$PWD/serializablesettings.cpp \
   $$PWD/reader.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/reader.h \
   $$PWD/serializablesettings.h \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h \
   $$PWD/settings_validators.hpp

include($$PWD/adapters/adapters.pri)