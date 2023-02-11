#ifndef COMMANDCALLBACK_H
#define COMMANDCALLBACK_H

#include <QtGlobal>
#include <type_traits>

namespace Radapter {

class Worker;
class Command;
class Reply;
class WorkerMsg;
struct CallbackConcept {
    void execute(const WorkerMsg &msg);
    virtual void *user() const = 0;
    virtual ~CallbackConcept() = default;
private:
    virtual void executePlain(const Command *cmd, const Reply *reply) {Q_UNUSED(cmd)Q_UNUSED(reply)};
    virtual void executeMsg(const WorkerMsg &msg) {Q_UNUSED(msg)};
};

// forward declaration
template <typename WantedCommand>
WantedCommand command_cast(const Command *command);

template<typename User, typename CommandT>
struct MethodCallback : CallbackConcept {
    static_assert(std::is_base_of<Radapter::Worker, User>(), "Must inherit Radapter::Worker!");
    using ReplyT = typename CommandT::WantedReply;
    using Callback = void (User::*)(const CommandT *, const ReplyT *);
    MethodCallback(User* user, Callback cb) : m_cb(cb), m_user(user) {}
    virtual void executePlain(const Command *cmd, const Reply *reply) override {
        (m_user->*m_cb)(command_cast<const CommandT*>(cmd), CommandT::replyCast(reply));
    }
    virtual void *user() const override {
        return m_user;
    }
    Callback m_cb;
    User* m_user;
};

template<typename User>
struct MethodMsgCallback : CallbackConcept {
    static_assert(std::is_base_of<Radapter::Worker, User>(), "Must inherit Radapter::Worker!");
    using Callback = void (User::*)(const WorkerMsg &);
    MethodMsgCallback(User* user, Callback cb) : m_cb(cb), m_user(user) {}
    virtual void executeMsg(const WorkerMsg &msg) override {
        (m_user->*m_cb)(msg);
    }
    virtual void *user() const override {
        return m_user;
    }
    Callback m_cb;
    User* m_user;
};

}


#endif // COMMANDCALLBACK_H
