#ifndef PARSING_PRIVATE_H
#define PARSING_PRIVATE_H
#include "private/global.h"
#include <QStringBuilder>
namespace Radapter{ namespace Private {

struct FuncResult {
    QString func;
    QStringList data;
};
FuncResult parseFunc(const QString &rawFunc);
bool isFunc(const QString &rawFunc);
QString normalizeFunc(const QString &rawFunc);
}}
#endif // PARSING_PRIVATE_H
