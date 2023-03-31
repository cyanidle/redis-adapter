#include "yamlreader.h"
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>
#include "jsondict/jsondict.hpp"
#include "yamlqttypesadapter.hpp"

using namespace YAML;
using namespace Settings;

YamlReader::YamlReader(const QString &dir, const QString &file, QObject *parent) :
    DictReader(dir, file, parent)
{
}

void YamlReader::recurseMapNode(const QStringList &prefix, const Node &who, JsonDict &output)
{
    for (const auto &node: who) {
        auto key = prefix;
        key.append(node.first.as<QString>());
        if (node.second.IsDefined() && node.second.IsScalar() && node.second.Tag() == "!include") {
            setPath(node.second.as<QString>());
            output[key] = getAll();
        }
        if (node.second.IsDefined() && node.second.IsMap()) {
            recurseMapNode(key, node.second, output);
        }
    }
}

QVariant YamlReader::getAll()
{
    static const QString extension(".yaml");
    auto pathWas = path();
    auto wantedPath = resource() + "/" + path();
    auto actualPath = wantedPath.endsWith(extension) ? wantedPath : wantedPath + extension;
    auto config = LoadFile(actualPath.toStdString());
    auto result = JsonDict(config.as<QVariantMap>());
    recurseMapNode({}, config, result);
    setPath(pathWas);
    return result.toVariant();
}
