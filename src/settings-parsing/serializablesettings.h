#ifndef SETTINGS_SERIALIZABLESETTINGS_H
#define SETTINGS_SERIALIZABLESETTINGS_H

#include "settings-parsing/serializer.hpp"
#define ROOT_PREFIX QStringLiteral("~")

namespace Settings {

class SerializableSettings : protected Serializer::SerializableGadget
{
    Q_GADGET
public:
    SerializableSettings();
    SerializableSettings *parent();
    const SerializableSettings *parent() const;
    virtual void deserialize(const QVariantMap &src) override;
    QString prefix() const;
private:
    void performCheck();
    void throwOnNonSettings(const Serializable *child) const;
    void updatePrefix(const QString &name);

    QString m_prefix{ROOT_PREFIX};
};

} // namespace Settings

#endif // SETTINGS_SERIALIZABLESETTINGS_H
