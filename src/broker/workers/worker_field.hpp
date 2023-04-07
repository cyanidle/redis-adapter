#ifndef WORKER_FIELD_HPP
#define WORKER_FIELD_HPP

#include "../broker.h"
#include "settings-parsing/serializablesettings.h"

namespace Settings {

template <typename Target>
struct FetchWorker : public Target {
    using typename Target::valueType;
    static_assert(
        std::is_pointer<valueType>() &&
        std::is_base_of<Radapter::Worker, typename std::remove_pointer<valueType>::type>(),
        "FetchWorker<Field<T>> can obly be used with Worker subclass pointers!"
        );
    using typename Target::valueRef;
    using Target::Target;
    using Target::operator=;
    using Target::operator==;
    bool updateWithVariant(const QVariant &source) {
        auto asStr = source.toString();
        if (asStr.isEmpty()) return false;
        auto found = Radapter::Broker::instance()->getWorker<valueType>(asStr);
        if (!found && !Target::attributes().contains(NON_REQUIRED_ATTR)) {
            throw std::runtime_error("Could not fetch worker: " + asStr.toStdString());
        }
        return Target::updateWithVariant(QVariant::fromValue(found));
    }
};

template <typename WorkerPtr>
using WorkerByNameRequired = FetchWorker<Required<WorkerPtr>>;
template <typename WorkerPtr>
using WorkerByNameNonRequired = FetchWorker<NonRequired<WorkerPtr>>;

}

#endif // WORKER_FIELD_HPP
