SOURCES+=
HEADERS+= \
   $$PWD/yaml.hpp \
   $$PWD/toml.hpp

include($$PWD/yaml/settings-parsing_adapters_yaml.pri)
include($$PWD/toml/settings-parsing_adapters_toml.pri)