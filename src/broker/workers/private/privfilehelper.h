#ifndef RADAPTER_PRIVATE_FILEHELPER_H
#define RADAPTER_PRIVATE_FILEHELPER_H

#include <QObject>
#include "jsondict/jsondict.h"

struct QFile;
namespace Radapter {
namespace Private {

class FileHelper : public QObject
{
    Q_OBJECT
public:
    explicit FileHelper(QFile *target, QObject *parent);
    void start();
signals:
    void jsonRead(const JsonDict &json);
    void error(const QString &reason);
private:
    void mainloop();

    QFile *m_target;
    QThread *m_thread;
};

} // namespace Private
} // namespace Radapter

#endif // RADAPTER_PRIVATE_FILEHELPER_H
