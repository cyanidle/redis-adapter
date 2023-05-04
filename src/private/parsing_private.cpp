#include "parsing_private.h"
#include <QRegularExpression>
Radapter::Private::FuncResult Radapter::Private::parseFunc(const QString &rawFunc)
{
    if (!isFunc(rawFunc)) {
        throw std::runtime_error("Attempt to parse invalid pipe function: " + rawFunc.toStdString());
    }
    auto split = rawFunc.split('(');
    auto func = split.takeFirst();
    auto rawData = split.join('(');
    auto data = QStringList();
    auto openCount = 1;
    auto closeCount = 0;
    QString current;
    current.reserve(16);
    for (auto &ch: rawData) {
        if (ch == ' ') continue;
        else if (ch == '(') {
            openCount++;
        } else if (ch == ')') {
            closeCount++;
            if (openCount == closeCount) {
                data.append(current);
                current.clear();
            }
        } else if(ch == ',') {
            if (openCount - closeCount == 1) {
                data.append(current);
                current.clear();
                continue;
            }
        }
        current+=ch;
    }
    return {func, data};
}

bool Radapter::Private::isFunc(const QString &rawFunc)
{
    static auto tester = QRegularExpression("\\w+(?:\\(.*\\))?");
    return tester.match(rawFunc).hasMatch();
}

QString Radapter::Private::normalizeFunc(const QString &rawFunc)
{
    auto copy = rawFunc;
    return copy.replace(' ', "");
}
