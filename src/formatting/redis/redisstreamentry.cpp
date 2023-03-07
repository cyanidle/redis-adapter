#include "redisstreamentry.h"

namespace Redis {

StreamEntry::StreamEntry(const QVariantList &source) {
    auto splitStreamId = source.first().toString().split("-");
    timestamp = splitStreamId.first().toULong();
    id = splitStreamId.last().toULong();
    auto asList = source.last().toList();
    for (auto i = 1; i <= asList.size(); i+=2) {
        values.insert(asList[i - 1].toString(), asList[i].toString());
    }
}

QString StreamEntry::streamId() const {
    return QString::number(timestamp) + "-" + QString::number(id);
}

} // namespace Redis
