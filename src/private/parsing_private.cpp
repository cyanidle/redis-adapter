#include "parsing_private.h"
#include <QRegularExpression>
#define PATTERN "\\(.*\\)"
Radapter::Private::FuncResult Radapter::Private::parseFunc(const QString &rawFunc)
{
    static auto splitter = QRegularExpression(PATTERN);
    if (rawFunc.size() < 2) {
        throw std::runtime_error("Attempt to parse invalid pipe function: " + rawFunc.toStdString());
    }
    if (!rawFunc.contains(splitter)) {
        return {rawFunc, {}};
    }
    auto func = rawFunc.split(splitter)[0];
    auto data = splitter
                    .match(rawFunc) // func(data, data)
                    .captured().remove(0, 1).chopped(1) // data, data
                    .split(','); // [data, data]
    for (auto &item: data){
        item = item.simplified();
    };
    return {func, data};
}

bool Radapter::Private::isFunc(const QString &rawFunc)
{
    static auto splitter = QRegularExpression(PATTERN);
    return splitter.match(rawFunc).hasMatch();
}

QString Radapter::Private::normalizeFunc(const QString &rawFunc)
{
    auto copy = rawFunc;
    return copy.replace(' ', "");
}
