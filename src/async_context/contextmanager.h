#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

#include <QHash>
#include "radapterlogging.h"
#include "private/global.h"
#include <stdexcept>

namespace Radapter {

struct ContextBase {
    using Handle = void*;
    using HandleInt = typename QIntegerForSizeof<Handle>::Unsigned;

    ContextBase() = default;
    void setHandle(Handle handle);
    static HandleInt handleToNumber(Handle handle);
    static Handle numberToHandle(HandleInt number);
    Handle handle();
    bool isDone() const;
    void setDone();
    virtual ~ContextBase() = default;
private:
    bool m_isDone{false};
    Handle m_handle{nullptr};
};

template <typename Context>
struct ContextManager {
    using Handle = ContextBase::Handle;
    using HandleInt = ContextBase::HandleInt;
    static HandleInt handleToNumber(Handle handle)
    {
        return ContextBase::handleToNumber(handle);
    }
    static Handle numberToHandle(HandleInt number)
    {
        return ContextBase::numberToHandle(number);
    }
    static_assert(std::is_base_of<ContextBase, Context>(), "Must inherit ContextBase!");
    ContextManager() = default;
    template <typename Derived = Context, typename...Args>
    Handle create(Args&&...args) {
        auto handle = incrCurrent();
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
    void clearAll();
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
    template <typename Predicate, typename...Args>
    void forEach(Predicate pred, Args&&...args) {
        auto iter = m_ctxs.begin();
        while (iter++ != m_ctxs.end()) {
            (iter.value()->*pred)(std::forward<Args>(args)...);
        }
    }
    ~ContextManager();
private:
    Handle incrCurrent();
    QHash<Handle, Context*> m_ctxs{};
    HandleInt m_current{1};
};

template<typename Context>
Context &ContextManager<Context>::get(Handle handle)
{
    if (!m_ctxs.contains(handle)) throw std::runtime_error("Context not found for handle: " +
                    QString::number(handleToNumber(handle)).toStdString());
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
void ContextManager<Context>::clearAll()
{
    qDeleteAll(m_ctxs);
    m_ctxs.clear();
}

template<typename Context>
ContextManager<Context>::~ContextManager()
{
    qDeleteAll(m_ctxs);
}

template<typename Context>
ContextBase::Handle ContextManager<Context>::incrCurrent()
{
    return numberToHandle(m_current++);
}

inline void ContextBase::setHandle(Handle handle)
{
    if (m_handle) throw std::runtime_error("Attempt to set already set handle");
    m_handle = handle;
}

inline QIntegerForSizeof<ContextBase::Handle>::Unsigned ContextBase::handleToNumber(Handle handle)
{
    return reinterpret_cast<HandleInt>(handle);
}

inline ContextBase::Handle ContextBase::numberToHandle(HandleInt number)
{
    return reinterpret_cast<Handle>(number);
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

}
#endif // CONTEXTMANAGER_H
