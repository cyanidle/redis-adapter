SOURCES+= \
   $$PWD/dictreader.cpp \
   $$PWD/reader.cpp \
   $$PWD/serializablesettings.cpp
HEADERS+= \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h \
   $$PWD/reader.h \
   $$PWD/serializablesettings.h \
   $$PWD/settings_validators.hpp

include($$PWD/adapters/adapters.pri)