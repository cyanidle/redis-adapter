#ifndef YAML_READER_H
#define YAML_READER_H

#include "private/global.h"
#include "../../dictreader.h"

namespace YAML {
class Node;
}
class JsonDict;
namespace Settings {

class RADAPTER_API YamlReader : public DictReader
{
    Q_OBJECT
public:
    YamlReader(const QString &dir = "conf", const QString &file = "config", QObject *parent = nullptr);
    QVariant getAll() override;
private:
    void recurseMapNode(const QStringList &prefix, const YAML::Node &who, JsonDict &output);
};

}

#endif
