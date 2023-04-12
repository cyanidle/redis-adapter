#ifndef RADAPTER_VALIDATORSPLUGIN_H
#define RADAPTER_VALIDATORSPLUGIN_H

#include "plugins/radapter_plugin.h"

namespace Radapter {

class RADAPTER_API ValidatorsPlugin : public Radapter::Plugin
{
    virtual QString name() const override;
    virtual QMap<Validator::Function, QStringList/*aliases*/> validators() const override;
};

} // namespace Radapter
#endif // RADAPTER_VALIDATORSPLUGIN_H
