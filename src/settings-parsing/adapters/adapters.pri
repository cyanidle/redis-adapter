SOURCES+=
HEADERS+= \
   $$PWD/toml.hpp \
   $$PWD/yaml.hpp

include($$PWD/toml/toml.pri)
include($$PWD/yaml/yaml.pri)
