SOURCES+= \
   $$PWD/serializable.cpp \
   $$PWD/common_validators.cpp \
   $$PWD/validator_fetch.cpp
HEADERS+= \
   $$PWD/validated.hpp \
   $$PWD/field_super.h \
   $$PWD/serializable.h \
   $$PWD/validator_fetch.h \
   $$PWD/utils.hpp \
   $$PWD/common_validators.h \
   $$PWD/bindable.hpp \
   $$PWD/common_fields.hpp

include($$PWD/private/private.pri)