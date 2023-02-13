#ifndef COMMANDCALLBACK_H
#define COMMANDCALLBACK_H

#include <QtGlobal>
#include <type_traits>
#include <stdexcept>

namespace Radapter {

// forward declarations
class Worker;
class Command;
class Reply;
class WorkerMsg;
template <typename WantedCommand>
WantedCommand command_cast(const Command *command);
template <typename WantedReply>
WantedReply reply_cast(const Reply *reply);

struct CallbackConcept {
    virtual void execute(const WorkerMsg &msg) const = 0;
    virtual Worker *worker() const = 0;
    virtual ~CallbackConcept() = default;

    static const Reply* replyFromMsg(const WorkerMsg &msg);
    static const Command* cmdFromMsg(const WorkerMsg &msg);
};

template<typename User, typename CommandT>
struct CallbackCmdReply : CallbackConcept {
    static_assert(std::is_base_of<Radapter::Worker, User>(), "Must inherit Radapter::Worker!");
    using ReplyT = typename CommandT::WantedReply;
    using Callback = void (User::*)(const CommandT *, const ReplyT *);
    CallbackCmdReply(User* user, Callback cb) : m_cb(cb), m_user(user) {
        if (!user) {
            throw std::invalid_argument("Nullptr user of a callback!");
        }
    }
    virtual void execute(const WorkerMsg &msg) const override final {
        (m_user->*m_cb)(command_cast<const CommandT*>(cmdFromMsg(msg)),
                        CommandT::replyCast(replyFromMsg(msg)));
    }
    virtual Worker *worker() const override final {
        return m_user;
    }
    Callback m_cb;
    User* m_user;
};

template<typename User>
struct CallbackMsg : CallbackConcept {
    static_assert(std::is_base_of<Radapter::Worker, User>(), "Must inherit Radapter::Worker!");
    using Callback = void (User::*)(const WorkerMsg &);
    CallbackMsg(User* user, Callback cb) : m_cb(cb), m_user(user) {
        if (!user) {
            throw std::invalid_argument("Nullptr user of a callback!");
        }
    }
    virtual void execute(const WorkerMsg &msg) const override final {
        (m_user->*m_cb)(msg);
    }
    virtual Worker *worker() const override final {
        return m_user;
    }
    Callback m_cb;
    User* m_user;
};

template<typename User, typename ReplyT>
struct CallbackReply : CallbackConcept {
    static_assert(std::is_base_of<Radapter::Worker, User>(), "Must inherit Radapter::Worker!");
    using Callback = void (User::*)(const ReplyT *);
    CallbackReply(User* user, Callback cb) : m_cb(cb), m_user(user) {
        if (!user) {
            throw std::invalid_argument("Nullptr user of a callback!");
        }
    }
    virtual void execute(const WorkerMsg &msg) const override final {
        (m_user->*m_cb)(reply_cast<const ReplyT*>(replyFromMsg(msg)));
    }
    virtual Worker *worker() const override final {
        return m_user;
    }
    Callback m_cb;
    User* m_user;
};
}


#endif // COMMANDCALLBACK_H
