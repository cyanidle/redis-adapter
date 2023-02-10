#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

#include <QHash>
#include "radapterlogging.h"
#include "private/global.h"
#include <stdexcept>

namespace Radapter {
struct ContextBase {
    ContextBase() = default;
    using Handle = void*;
    void setHandle(Handle handle);
    Handle handle();
    bool isDone() const;
    void setDone();
    template<class Derived>
    Derived *as();
    template<class Derived>
    const Derived *as() const;
    virtual ~ContextBase() = default;
private:
    bool m_isDone{false};
    Handle m_handle{nullptr};
};

template <typename Context>
struct ContextManager {
    using Handle = ContextBase::Handle;
    static_assert(std::is_base_of<ContextBase, Context>(), "Must inherit ContextBase!");
    ContextManager() = default;
    template <typename Derived = Context, typename...Args>
    Handle create(Args&&...args) {
        auto handle = currentNext();
        m_ctxs[handle] = new Derived(std::forward<Args>(args)...);
        m_ctxs[handle]->setHandle(handle);
        return handle;
    }
    template <typename Derived = Context, typename...Args>
    Handle create(Handle handle, Args&&...args) {
        if (m_ctxs.contains(handle)) {
            throw std::invalid_argument("Shadowing handle!");
        }
        if (!handle) {
            throw std::invalid_argument("Zero value handle!");
        }
        m_ctxs[handle] = new Derived(std::forward<Args>(args)...);
        m_ctxs[handle]->setHandle(handle);
        return handle;
    }
    Context &get(Handle handle);
    const Context &get(Handle handle) const;
    void remove(Handle handle);
    void remove(const Context &context);
    template <typename Predicate, typename...Args>
    Handle getBasedOn(Predicate pred, Args&&...args) {
        auto iter = m_ctxs.begin();
        while (iter != m_ctxs.end()) {
            if ((iter.value()->*pred)(std::forward<Args>(args)...)) {
                return iter.key();
            }
            ++iter;
        }
        return nullptr;
    }
    void clearDone();
    template <typename Predicate, typename...Args>
    void clearBasedOn(Predicate pred, Args&&...args) {
        auto iter = m_ctxs.begin();
        while (iter != m_ctxs.end()) {
            if ((iter.value()->*pred)(std::forward<Args>(args)...)) {
                delete *iter;
                iter = m_ctxs.erase(iter);
            } else {
                ++iter;
            }
        }
    }
    ~ContextManager();
private:
    Handle currentNext() const;
    QHash<Handle, Context*> m_ctxs{};
};

template<typename Context>
Context &ContextManager<Context>::get(Handle handle)
{
    if (!m_ctxs.contains(handle)) brokerWarn() << "Context not found for handle: " << handle;
    return *m_ctxs[handle];
}

template<typename Context>
const Context &ContextManager<Context>::get(Handle handle) const
{
    if (!m_ctxs.contains(handle)) throw std::invalid_argument("Nonexistant context");
    return *m_ctxs.value(handle);
}

template<typename Context>
void ContextManager<Context>::remove(Handle handle)
{
    if (m_ctxs.contains(handle)) {
        delete m_ctxs[handle];
        m_ctxs.remove(handle);
    }
}

template<typename Context>
void ContextManager<Context>::remove(const Context &context)
{
    remove(context.handle());
}

template<typename Context>
void ContextManager<Context>::clearDone()
{
    clearBasedOn(&ContextBase::isDone);
}

template<typename Context>
ContextManager<Context>::~ContextManager()
{
    qDeleteAll(m_ctxs);
}

template<typename Context>
ContextBase::Handle ContextManager<Context>::currentNext() const
{
    using HandleInt = typename QIntegerForSizeof<Handle>::Unsigned;
    return reinterpret_cast<Handle>(static_cast<HandleInt>(m_ctxs.size() + 1));
}

inline void ContextBase::setHandle(Handle handle)
{
    if (m_handle) throw std::runtime_error("Attempt to set already set handle");
    m_handle = handle;
}

inline ContextBase::Handle ContextBase::handle()
{
    return m_handle;
}

inline bool ContextBase::isDone() const
{
    return m_isDone;
}

inline void ContextBase::setDone()
{
    m_isDone = true;
}

template<class Derived>
Derived *ContextBase::as()
{
    return static_cast<Derived *>(static_cast<void*>(this));
}

template<class Derived>
const Derived *ContextBase::as() const
{
    return static_cast<const Derived *>(static_cast<const void*>(this));
}

}
#endif // CONTEXTMANAGER_H
