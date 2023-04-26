SOURCES+= \
   $$PWD/reader.cpp \
   $$PWD/serializablesetting.cpp \
   $$PWD/settings_validators.cpp \
   $$PWD/dictreader.cpp \
   $$PWD/settingsexample.cpp
HEADERS+= \
   $$PWD/private/impl_settingscomment.h \
   $$PWD/serializablesetting.h \
   $$PWD/settings_validators.h \
   $$PWD/reader.h \
   $$PWD/convertutils.hpp \
   $$PWD/dictreader.h \
   $$PWD/settingsexample.h

include($$PWD/adapters/adapters.pri)