#include "producerfilter.h"
#include "filters/producerfiltersettings.hpp"

using namespace Radapter;

struct ProducerFilter::Private {
    Settings::ProducerFilter settings;
    ProducerFilter::Strategy strategy;
    QMap<QString, double> all_fields;
    Radapter::WorkerMsg last;
};

ProducerFilter::ProducerFilter(const Settings::ProducerFilter &settings) :
    d(new Private)
{
    d->strategy = settings.by_wildcard->isEmpty() ? StrategyByWildcard : StrategyStrict;
    for (auto [field, val] : settings.by_field) {
        d->all_fields.insert(field, val);
    }
}

Radapter::Interceptor *ProducerFilter::newCopy() const
{
    return new ProducerFilter(d->settings);
}

ProducerFilter::~ProducerFilter()
{
    delete d;
}

void ProducerFilter::onMsgFromWorker(Radapter::WorkerMsg &msg)
{
    if (msg.testFlags(Radapter::WorkerMsg::MsgReply |
                      Radapter::WorkerMsg::MsgCommand)) {
        emit msgFromWorker(msg);
        return;
    }
    if (d->strategy == StrategyByWildcard) {
        addFiltersByWildcard(msg);
    }
    filterStrictByName(msg);
    d->last.merge(msg);
}

void ProducerFilter::filterStrictByName(Radapter::WorkerMsg &srcMsg)
{
    if (d->last.isEmpty()) {
        emit msgFromWorker(srcMsg);
    }
    bool shouldAdd = false;
    for (auto &item : srcMsg) {
        auto currentValue = item.value();
        auto lastValue = d->last[item.key()];
        auto currentKey = item.key().join(":");
        if (!d->all_fields.contains(currentKey)) {
            shouldAdd = true;
            break;
        }
        if (!currentValue.canConvert<double>() || !lastValue.canConvert<double>()) {
            shouldAdd = true;
            break;
        }
        if (qAbs(currentValue.toDouble() - lastValue.toDouble()) > d->all_fields[currentKey]) {
            shouldAdd = true;
            break;
        }
    }
    if (shouldAdd) {
        emit msgFromWorker(srcMsg);
    }
}

void ProducerFilter::addFiltersByWildcard(const JsonDict &cachedJson)
{
    auto cachedKeys = cachedJson.flatten(":").keys();
    for (auto &key : cachedKeys) {
        if (d->all_fields.contains(key)) {
            continue;
        }
        for (auto [glob, val]: d->settings.by_wildcard)
        {
            auto globRe = QRegularExpression(QRegularExpression::wildcardToRegularExpression(glob));
            if (globRe.globalMatch(key).isValid()) {
                d->all_fields.insert(key, val);
            }
        }
    }
}
