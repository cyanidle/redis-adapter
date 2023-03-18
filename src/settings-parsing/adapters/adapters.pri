SOURCES+=
HEADERS+= \
   $$PWD/yaml.hpp \
   $$PWD/toml.hpp

include($$PWD/yaml/yaml.pri)
include($$PWD/toml/toml.pri)