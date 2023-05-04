SOURCES+= \
   $$PWD/metainfopipe.cpp \
   $$PWD/remappingpipe.cpp \
   $$PWD/renamingpipe.cpp \
   $$PWD/validatinginterceptor.cpp \
   $$PWD/namespacewrapper.cpp \
   $$PWD/namespaceunwrapper.cpp \
   $$PWD/duplicatinginterceptor.cpp
HEADERS+= \
   $$PWD/namespacewrapper.h \
   $$PWD/remappingpipe.h \
   $$PWD/renamingpipe.h \
   $$PWD/validatinginterceptor.h \
   $$PWD/namespaceunwrapper.h \
   $$PWD/duplicatinginterceptor.h \
   $$PWD/metainfopipe.h

include($$PWD/settings/settings.pri)