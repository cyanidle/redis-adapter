#include "privfilehelper.h"
#include "templates/algorithms.hpp"
#include <QJsonDocument>
#include <QThread>
#include <QFile>

namespace Radapter {
namespace Private {

FileHelper::FileHelper(QIODevice *target, QObject *parent) :
    QObject{},
    m_target(target),
    m_thread(new QThread(parent))
{
    connect(m_thread, &QThread::started, this, &FileHelper::mainloop);
    connect(m_thread, &QThread::finished, this, &QObject::deleteLater);
    moveToThread(m_thread);
}

void FileHelper::start()
{
    m_thread->start();
}
void FileHelper::mainloop()
{
    auto f = m_target;
    QByteArray arr;
    quint64 currentEnd = 0;
    arr.reserve(2048);
    char buf[2048];
    QJsonParseError err;
    while ( true )
    {
        f->waitForReadyRead(1000);
        auto count = f->read(buf, sizeof(buf));
        if (!count) continue;
        if (count == -1) {
            emit error("File permissions/File size/Unknown");
            return;
        }
        arr.append(buf, count);
        auto openCount = 0;
        auto closeCount = 0;
        auto startPos = 0;
        for (int i = 0; i < arr.size(); ++i) {
            auto &byte = arr[i];
            if (byte == '{') {
                if (!openCount) {
                    startPos = i;
                }
                openCount++;
            }
            if (byte == '}') closeCount++;
            if (openCount && openCount == closeCount) {
                openCount = closeCount = 0;
                auto currentObj = arr.mid(startPos, i - startPos + 1);
                auto json = JsonDict::fromBytes(currentObj, &err);
                if (err.error != QJsonParseError::NoError) {
                    emit error("Json Parsing. Details: " + err.errorString());
                } else {
                    emit jsonRead(json);
                }
                currentEnd = i;
            }
        }
        arr = arr.mid(currentEnd + 1);
        currentEnd = 0;
    }
}

} // namespace Private
} // namespace Radapter
