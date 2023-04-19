#ifndef RADAPTER_PLUGIN_H
#define RADAPTER_PLUGIN_H

#include "private/global.h"
#include <type_traits>

#define PLUGIN_LOADER_FUNCTION_NAME "radapterFetchPlugin"
namespace Validator{
using Function = bool (*)(QVariant &target, const QVariantList &args, QVariant& state);
}
namespace Radapter {
class Interceptor;
class Plugin {
public:
    virtual QString name() const = 0;
    virtual QMap<Validator::Function, QStringList/*aliases*/> validators() const = 0;
    virtual ~Plugin() = default;
};
typedef ::Radapter::Plugin* (*PluginLoaderFunction)();
}

#define RADAPTER_DECLARE_PLUGIN(cls) \
extern "C" RADAPTER_API ::Radapter::Plugin *radapterFetchPlugin() { \
static_assert(std::is_base_of<::Radapter::Plugin, cls>(), "Plugins must inherit Radapter::Plugin!");\
return new cls; \
}

#endif //RADAPTER_PLUGIN_H
