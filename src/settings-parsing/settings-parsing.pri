SOURCES+= \
   $$PWD/reader.cpp \
   $$PWD/serializablesetting.cpp \
   $$PWD/settings_validators.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/serializablesetting.h \
   $$PWD/settings_validators.h \
   $$PWD/reader.h \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h

include($$PWD/adapters/adapters.pri)