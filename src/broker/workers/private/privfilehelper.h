#ifndef RADAPTER_PRIVATE_FILEHELPER_H
#define RADAPTER_PRIVATE_FILEHELPER_H

#include <QObject>
#include "jsondict/jsondict.h"

namespace Radapter {
namespace Private {

class FileHelper : public QObject
{
    Q_OBJECT
    struct Private;
public:
    explicit FileHelper(QIODevice *target, QObject *parent);
    void start();
    ~FileHelper();
signals:
    void jsonRead(const JsonDict &json);
    void error(const QString &reason);
private:
    void mainloop();

    Private *d;
};

} // namespace Private
} // namespace Radapter

#endif // RADAPTER_PRIVATE_FILEHELPER_H
