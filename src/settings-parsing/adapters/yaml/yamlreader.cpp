#include "yamlreader.h"
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>
#include "jsondict/jsondict.h"
#include "templates/algorithms.hpp"
#include <QStringBuilder>
#include "yamlqttypesadapter.hpp"

using namespace YAML;
using namespace Settings;
using namespace Radapter;

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
            auto toInclude = node.second.as<QString>();
            if (toInclude.isEmpty()) {
                throw std::runtime_error("Cannot have !include statement with empty string/nonstring");
            }
            setPath(toInclude);
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
    auto asMap = config.as<QVariantMap>();
    auto result = JsonDict(asMap, false);
    recurseMapNode({}, config, result);
    setPath(pathWas);
    return result.toVariant();
}

void dispatch(YAML::Emitter &out, const QString &name, const FieldExample &example);

void formatNestedField(YAML::Emitter &out, const QString &name, const Example &example)
{
    out << name.toStdString();
    if (!example.comment.isEmpty()) {
        out << YAML::Comment(example.comment.toStdString());
    }
    out << YAML::BeginMap;
    for (auto [field, fieldExample]: keyVal(example.fields)) {
        dispatch(out, field, fieldExample);
    }
    out << YAML::EndMap;
}

void formatSeq(YAML::Emitter &out, const QString &name, const Example &example)
{
    out << YAML::Key << name.toStdString();
    if (!example.comment.isEmpty()) {
        out << YAML::Comment(example.comment.toStdString());
    }
    out << YAML::BeginSeq;
        out << YAML::BeginMap;
            for (auto [field, fieldExample]: keyVal(example.fields)) {
                dispatch(out, field, fieldExample);
            }
        out << YAML::EndMap;
    out << YAML::EndSeq;
}

void formatMap(YAML::Emitter &out, const QString &name, const Example &example)
{
    out << YAML::Key << name.toStdString();
    if (!example.comment.isEmpty()) {
                out << YAML::Comment(example.comment.toStdString());
    }
    out << YAML::BeginMap;
        out << "<name>";
        out << YAML::BeginMap;
            for (auto [field, fieldExample]: keyVal(example.fields)) {
                dispatch(out, field, fieldExample);
            }
        out << YAML::EndMap;
    out << YAML::EndMap;
}

void formatNonNested(YAML::Emitter &out, const QString &name, const FieldExample &example)
{
    auto replaceFirst = [](QString &target, const QString &what, const QString &with){
        target.replace(target.indexOf(what), what.size(), with);
    };
    auto defaultToPrint = example.defaultValue.toString();
    auto typeName = example.typeName;
    QString commentToPrint;
    if (!defaultToPrint.isEmpty()) commentToPrint += '(' % defaultToPrint % "). ";
    if (!example.attributes.isEmpty()) commentToPrint += '[' % example.attributes.join(", ") % "] ";
    commentToPrint += example.comment;
    auto comment = YAML::Comment(commentToPrint.toStdString());
    out << YAML::Key << name.toStdString();
    if (example.fieldType == FieldExample::FieldPlain) {
        out << YAML::Value << typeName.toStdString();
        if (!commentToPrint.isEmpty()) out << comment;
    } else if (example.fieldType == FieldExample::FieldSequence) {
        auto was = typeName;
        replaceFirst(typeName, "list<", "");
        if (was != typeName) {
            typeName.chop(1);
        }
        out << YAML::BeginSeq;
            out << YAML::Value << typeName.toStdString();
            if (!commentToPrint.isEmpty()) out << comment;
        out << YAML::EndSeq;
    } else if (example.fieldType == FieldExample::FieldMapping) {
        auto was = typeName;
        replaceFirst(typeName, "map<string,", "");
        if (was != typeName) {
            typeName.chop(1);
        }
        out << YAML::BeginMap;
            out<< YAML::Key << "<name>" << YAML::Value << typeName.toStdString();
            if (!commentToPrint.isEmpty()) out << comment;
        out << YAML::EndMap;
    }
}

void dispatch(YAML::Emitter &out, const QString &name, const FieldExample &example)
{
    switch(example.nestedType) {
    case FieldExample::None: formatNonNested(out, name, example); return;
    case FieldExample::NestedField: formatNestedField(out, name, *example.nested); return;
    case FieldExample::NestedSequence: formatSeq(out, name, *example.nested); return;
    case FieldExample::NestedMapping: formatMap(out, name, *example.nested); return;
    };
}

QString YamlReader::formatExample(const Example &example) const
{
    std::stringstream out;
    Emitter emitter(out);
    emitter << YAML::BeginMap;
    for (auto [field, fieldExample]: keyVal(example.fields)) {
        dispatch(emitter, field, fieldExample);
    }
    emitter << YAML::EndMap;
    auto res = QString::fromStdString(out.str());
    return res;
}















