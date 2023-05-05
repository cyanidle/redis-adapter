#include "fileworker.h"
#include "settings/fileworkersettings.h"
#include "broker/workers/private/workermsg.h"
#include "jsondict/jsondict.h"
#include "radapterlogging.h"
#include <QDir>
#include "broker/workers/worker.h"
#include <QDateTime>
#include "private/privfilehelper.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>
#include <QFile>
#include <QTimer>

using namespace Radapter;

struct Radapter::FileWorker::Private
{
    QFile *file;
    Settings::FileWorker settings;
    bool writeEnabled;
    bool readEnabled;
    ::Radapter::Private::FileHelper *helper;
};

FileWorker::FileWorker(const Settings::FileWorker &settings, QThread *thread) :
    Radapter::Worker(settings.worker, thread),
    d(new FileWorker::Private{
        /*.file*/ new QFile(this),
        /*.settings*/ settings,
        /*.writeCount*/ 0,
        /*.writeEnabled*/ false,
        /*.helper*/ nullptr
    })
{
    if (settings.filepath != "stdin" && settings.filepath != "stdout" && settings.filepath->contains('/')) {
        QDir().mkpath(settings.filepath->left(settings.filepath->lastIndexOf('/')));
    }
    if (!checkOpened()) {
        throw std::runtime_error(std::string("Could not open file: ") + filepath().toStdString());
    }
    if (d->writeEnabled && !d->file->size()) {
        d->file->write("[\n");
    }
    connect(this, &Worker::connectedToConsumer, this, [this]{
        if (d->readEnabled) {
            startReading();
            return;
        }
        if (wasStarted()) {
            workerError(this) << "Attempt to read from write only file:" << d->settings.filepath;
        } else {
            throw std::runtime_error("Attempt to read from write only file: " + d->settings.filepath->toStdString());
        }
    });
    connect(this, &Worker::connectedToProducer, this, [this]{
        if (d->writeEnabled) return;
        if (wasStarted()) {
            workerError(this) << "Attempt to write to read only file:" << d->settings.filepath;
        } else {
            throw std::runtime_error("Attempt to write to read only file: " + d->settings.filepath->toStdString());
        }
    });
}

const QString &FileWorker::filepath() const
{
    return d->settings.filepath;
}

FileWorker::~FileWorker()
{
    delete d;
}

void FileWorker::appendToFile(const JsonDict &info)
{
    if (!d->writeEnabled) {
        workerError(this) << "Attempt to write to read only file:" << d->settings.filepath;
        return;
    }
    if (!checkOpened()) {
        workerError(this) << "Could not open file with name: " << d->file->fileName();
        return;
    }
    QTextStream out(d->file);
    out << info.toBytes(d->settings.format);
}

bool FileWorker::checkOpened()
{
    if (d->file->isOpen()) return true;
    if (d->settings.filepath.value == "stdout") {
        d->writeEnabled = true;
        return d->file->open(stdout, QIODevice::WriteOnly);
    } else if (d->settings.filepath.value == "stdin") {
        d->readEnabled = true;
        return d->file->open(stdin, QIODevice::ReadOnly);
    } else {
        d->readEnabled = d->writeEnabled = true;
        d->file->setFileName(d->settings.filepath);
        return d->file->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);
    }
}

void FileWorker::onMsg(const Radapter::WorkerMsg &msg)
{
    appendToFile(msg);
}

void FileWorker::startReading()
{
    d->helper = new ::Radapter::Private::FileHelper(d->file, this);
    connect(d->helper, &::Radapter::Private::FileHelper::jsonRead, this, &FileWorker::send);
    connect(d->helper, &::Radapter::Private::FileHelper::error, this, [this](const QString &reason){
        workerError(this) << "Error occured! Reason:" << reason;
    });
    d->helper->start();
}

