#ifndef COMMANDCALLBACK_H
#define COMMANDCALLBACK_H

#include "templates/callable_info.hpp"
#include <QtGlobal>
#include <functional>
#include <type_traits>

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

struct CommandCallback {
    CommandCallback() = default;
    operator bool() const;
    CommandCallback(Worker *context, std::function<void(const WorkerMsg&)> cb);
    template<typename User>
    CommandCallback(User* user, void (User::*cb)(const WorkerMsg &)) :
        m_ctx(user),
        m_cb([user, cb](const WorkerMsg& msg){
            (user->*cb)(msg);
        })
    {}
    template<typename CommandT>
    CommandCallback(Worker *context, std::function<void(const CommandT*, const typename CommandT::WantedReply*)> cb) :
        m_ctx(context),
        m_cb([cb](const WorkerMsg &msg){
            cb(command_cast<const CommandT*>(cmdFromMsg(msg)), CommandT::replyCast(replyFromMsg(msg)));
        })
    {}
    template<typename CommandT, typename ReplyT>
    CommandCallback(Worker *context, std::function<void(const CommandT*, ReplyT*)> cb) :
        m_ctx(context),
        m_cb([cb](const WorkerMsg &msg){
            cb(command_cast<const CommandT*>(cmdFromMsg(msg)), reply_cast<const ReplyT*>(replyFromMsg(msg)));
        })
    {}
    template<typename User, typename CommandT, typename ReplyT>
    CommandCallback(User *user, void (User::*cb)(const CommandT*, const ReplyT*)) :
        m_ctx(user),
        m_cb([user, cb](const WorkerMsg &msg){
            (user->*cb)(command_cast<const CommandT*>(cmdFromMsg(msg)), reply_cast<const ReplyT*>(replyFromMsg(msg)));
        })
    {}
    template<typename User, typename CommandT>
    CommandCallback(User* user, void (User::*cb)(const CommandT*, const typename CommandT::WantedReply*)) :
        m_ctx(user),
        m_cb([user, cb] (const WorkerMsg &msg) {
            (user->*cb)(command_cast<const CommandT*>(cmdFromMsg(msg)), CommandT::replyCast(replyFromMsg(msg)));
        })
    {}
    template<typename ReplyT>
    CommandCallback(Worker *context, std::function<void(const ReplyT *)> cb) :
        m_ctx(context),
        m_cb([cb](const WorkerMsg &msg){
            cb(reply_cast<const ReplyT*>(replyFromMsg(msg)));
        })
    {}
    template<typename User, typename ReplyT>
    CommandCallback(User* user, void (User::*cb)(const ReplyT*)) :
        m_ctx(user),
        m_cb([user, cb] (const WorkerMsg &msg) {
            (user->*cb)(reply_cast<const ReplyT*>(replyFromMsg(msg)));
        })
    {}
    CommandCallback(Worker *context, std::function<void()> cb) :
        m_ctx(context),
        m_cb([cb](const WorkerMsg &msg){
            Q_UNUSED(msg)
            cb();
        })
    {}
    template<typename User>
    CommandCallback(User* user, void (User::*cb)()) :
        m_ctx(user),
        m_cb([user, cb] (const WorkerMsg &msg) {
            Q_UNUSED(msg)
            (user->*cb)();
        })
    {}
    void execute(const WorkerMsg &msg) const;
    Worker *worker() const;
    template<typename User, typename Slot>
    static CommandCallback fromAny(User *user, Slot&&slot, typename std::enable_if<
                                                                CallableInfo<Slot>::IsLambda &&
                                                                std::is_copy_constructible_v<Slot>
                                                                                    >::type* = nullptr) {
        return CommandCallback(user, typename LambdaInfo<Slot>::AsStdFunction(std::forward<Slot>(slot)));
    }
    template<typename User, typename Slot>
    static CommandCallback fromAny(User *user, Slot&&slot, typename std::enable_if<CallableInfo<Slot>::IsFunction ||
                                                                                    CallableInfo<Slot>::IsMethod>::type* = nullptr) {
        return CommandCallback(user, std::forward<Slot>(slot));
    }
protected:
    static const Reply* replyFromMsg(const WorkerMsg &msg);
    static const Command* cmdFromMsg(const WorkerMsg &msg);
private:
    Worker *m_ctx;
    std::function<void(const WorkerMsg&)> m_cb;
};

}

#endif // COMMANDCALLBACK_H
