SOURCES+= \
   $$PWD/duplicatinginterceptor.cpp \
   $$PWD/metainfopipe.cpp \
   $$PWD/namespaceunwrapper.cpp \
   $$PWD/namespacewrapper.cpp \
   $$PWD/remappingpipe.cpp \
   $$PWD/renamingpipe.cpp \
   $$PWD/validatinginterceptor.cpp
HEADERS+= \
   $$PWD/duplicatinginterceptor.h \
   $$PWD/metainfopipe.h \
   $$PWD/namespaceunwrapper.h \
   $$PWD/namespacewrapper.h \
   $$PWD/remappingpipe.h \
   $$PWD/renamingpipe.h \
   $$PWD/validatinginterceptor.h

include($$PWD/settings/settings.pri)