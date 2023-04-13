#ifndef RADAPTER_PIPESTART_H
#define RADAPTER_PIPESTART_H

#include <QObject>

namespace Radapter {
class WorkerMsg;
class Worker;
class WorkerProxy;
class PipeStart : public QObject
{
    Q_OBJECT
signals:
    void msgFromWorker(Radapter::WorkerMsg &msg);
public slots:
    void onSendMsg(const Radapter::WorkerMsg &msg);
public:
    explicit PipeStart(WorkerProxy *parent);
};

} // namespace Radapter

#endif // RADAPTER_PIPESTART_H
