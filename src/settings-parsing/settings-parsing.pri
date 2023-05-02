SOURCES+= \
   $$PWD/settingsexample.cpp \
   $$PWD/reader.cpp \
   $$PWD/settings_validators.cpp \
   $$PWD/serializablesetting.cpp \
   $$PWD/dictreader.cpp
HEADERS+= \
   $$PWD/serializablesetting.h \
   $$PWD/settings_validators.h \
   $$PWD/reader.h \
   $$PWD/convertutils.hpp \
   $$PWD/settingsexample.h \
   $$PWD/dictreader.h

include($$PWD/adapters/adapters.pri)
include($$PWD/private/private.pri)