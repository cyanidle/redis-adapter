#include "yamlreader.h"
#include <yaml-cpp/yaml.h>
#include "yamlqttypesadapter.hpp"

using namespace YAML;
using namespace Settings;

YamlReader::YamlReader(QString path, QObject *parent) :
    DictReader(path, parent)
{

}

QVariantMap YamlReader::parse()
{
    auto result = m_config.as<QVariantMap>();
    return result;
}

void YamlReader::onPathSet()
{
    m_config = LoadFile(path().toStdString() + ".yaml");
}

