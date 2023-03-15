#ifndef YAML_READER_H
#define YAML_READER_H

#include "private/global.h"
#include "../../dictreader.h"
#include <QVariantMap>
#include <yaml-cpp/node/node.h>

namespace Settings {

class RADAPTER_SHARED_SRC YamlReader : public DictReader
{
    Q_OBJECT
public:
    YamlReader(QString path, QObject *parent);
    QVariantMap parse() override;
protected:
    void onPathSet() override;

    YAML::Node m_config;
};

}

#endif
