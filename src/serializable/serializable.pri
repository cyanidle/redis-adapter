SOURCES+= \
   $$PWD/common_validators.cpp \
   $$PWD/serializable.cpp \
   $$PWD/validator_fetch.cpp
HEADERS+= \
   $$PWD/common_validators.h \
   $$PWD/validated.hpp \
   $$PWD/serializable.h \
   $$PWD/bindable.hpp \
   $$PWD/common_fields.hpp \
   $$PWD/validator_fetch.h

include($$PWD/private/private.pri)