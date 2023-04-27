SOURCES+= \
   $$PWD/dictreader.cpp \
   $$PWD/reader.cpp \
   $$PWD/serializablesetting.cpp \
   $$PWD/settingsexample.cpp \
   $$PWD/settings_validators.cpp
HEADERS+= \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h \
   $$PWD/reader.h \
   $$PWD/serializablesetting.h \
   $$PWD/settingsexample.h \
   $$PWD/settings_validators.h

include($$PWD/adapters/adapters.pri)
include($$PWD/private/private.pri)