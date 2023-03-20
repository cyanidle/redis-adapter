#ifndef YAML_READER_H
#define YAML_READER_H

#include "private/global.h"
#include "../../dictreader.h"
#include <QVariantMap>
#include <yaml-cpp/node/node.h>

namespace Settings {

class RADAPTER_API YamlReader : public DictReader
{
    Q_OBJECT
public:
    YamlReader(const QString &dir, const QString &file, QObject *parent);
    QVariant getAll() override;
};

}

#endif
