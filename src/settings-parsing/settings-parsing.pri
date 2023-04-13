SOURCES+= \
   $$PWD/serializablesettings.cpp \
   $$PWD/reader.cpp \
   $$PWD/settings_validators.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/settings_validators.h \
   $$PWD/reader.h \
   $$PWD/serializablesettings.h \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h

include($$PWD/adapters/adapters.pri)