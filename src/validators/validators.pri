SOURCES+= \
   $$PWD/common_validators.cpp \
   $$PWD/validator_fetch.cpp
HEADERS+= \
   $$PWD/validated_field.hpp \
   $$PWD/validator_fetch.h \
   $$PWD/common_validators.h

include($$PWD/private/private.pri)