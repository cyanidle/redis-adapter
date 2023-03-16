#include "yamlreader.h"
#include <yaml-cpp/yaml.h>
#include "jsondict/jsondict.hpp"
#include "yamlqttypesadapter.hpp"

using namespace YAML;
using namespace Settings;

YamlReader::YamlReader(const QString &dir, const QString &file, QObject *parent) :
    DictReader(dir, file, parent)
{
}

QVariant YamlReader::getAll()
{
    static const QString extension(".yaml");
    auto pathWas = path();
    auto wantedPath = resource() + "/" + path();
    auto actualPath = wantedPath.endsWith(extension) ? wantedPath : wantedPath + extension;
    auto config = LoadFile(actualPath.toStdString());
    auto result = JsonDict(config.as<QVariantMap>());
    for (const auto &node: config) {
        if (node.second.IsDefined() && node.second.Tag() == "!include") {
            setPath(node.second.as<QString>());
            result[node.first.as<QString>()] = getAll();
        }
    }
    setPath(pathWas);
    return result.toVariant();
}
