#include "privfilehelper.h"
#include "templates/algorithms.hpp"
#include <QJsonDocument>
#include <QThread>
#include <QFile>
#include <QStringBuilder>

namespace Radapter {
namespace Private {
struct FileHelper::Private {
    QIODevice *target;
    QByteArray arr;
    QJsonParseError err;
};

FileHelper::FileHelper(QIODevice *target, QObject *parent) :
    QObject{parent},
    d(new Private{})
{
    d->arr.reserve(2048);
    d->target = target;
}

void FileHelper::start()
{
    connect(d->target, &QIODevice::readyRead, this, &FileHelper::mainloop);
    mainloop();
}

FileHelper::~FileHelper()
{
    delete d;
}

void FileHelper::mainloop()
{
    while (d->target->bytesAvailable()) {
        d->arr = d->target->readLine();
        auto json = JsonDict::fromBytes(d->arr, &d->err);
        if (d->err.error != QJsonParseError::NoError) {
            emit error("Json Parsing. Details: "%d->err.errorString()%". Full: "%d->arr);
        } else {
            emit jsonRead(json);
        }
    }
}

} // namespace Private
} // namespace Radapter
