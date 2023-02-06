#ifndef METATYPES_H
#define METATYPES_H

#include "jsondict/jsondict.hpp"
#include "broker/worker/workermsg.h"

namespace Radapter {
    class Metatypes {
    public:
        static int WorkerMsgId;
        static int JsonDictId;
        static void registerAll() {
            JsonDictId = qRegisterMetaType<JsonDict>();
            WorkerMsgId = qRegisterMetaType<WorkerMsg>();
        }
        template <typename T>
        static constexpr bool isRegistered() {
            return QMetaTypeId2<T>::Defined;
        }
        template <typename T, typename Ret>
        using enable_meta_t = typename std::enable_if<isRegistered<T>(), Ret>::type;
    };
}

#endif // METATYPES_H
