SOURCES+= \
   $$PWD/common_validators.cpp \
   $$PWD/validator_fetch.cpp
HEADERS+= \
   $$PWD/common_validators.h \
   $$PWD/validated_field.hpp \
   $$PWD/validator_fetch.h

include($$PWD/private/private.pri)