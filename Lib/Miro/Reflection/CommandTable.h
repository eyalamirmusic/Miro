#pragma once

#include "../IgnoreUnused.h"
#include "../JSON/Json.h"
#include "Reflector.h"
#include "Serialize.h"

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace Miro
{
// Invoked exactly once per dispatch: error == nullptr means success,
// otherwise it points at the failure message.
using Resolve = std::function<void(const JSON& result, const std::string* error)>;

namespace Detail
{
class CompleterState
{
public:
    explicit CompleterState(Resolve resolveToUse)
        : resolve(std::move(resolveToUse))
    {
    }

    CompleterState(const CompleterState&) = delete;
    CompleterState& operator=(const CompleterState&) = delete;

    ~CompleterState()
    {
        auto message = std::string {"command handler dropped without completing"};
        settle(JSON {}, &message);
    }

    void settle(const JSON& result, const std::string* error)
    {
        if (!done.exchange(true))
            resolve(result, error);
    }

private:
    Resolve resolve;
    std::atomic_bool done {false};
};
} // namespace Detail

template <typename Res>
class Completer
{
public:
    explicit Completer(Resolve resolveToUse)
        : state(std::make_shared<Detail::CompleterState>(std::move(resolveToUse)))
    {
    }

    void resolve(const Res& value) const { state->settle(toJSON(value), nullptr); }

    void reject(const std::string& message) const
    {
        state->settle(JSON {}, &message);
    }

private:
    std::shared_ptr<Detail::CompleterState> state;
};

template <>
class Completer<void>
{
public:
    explicit Completer(Resolve resolveToUse)
        : state(std::make_shared<Detail::CompleterState>(std::move(resolveToUse)))
    {
    }

    void resolve() const { state->settle(JSON {}, nullptr); }

    void reject(const std::string& message) const
    {
        state->settle(JSON {}, &message);
    }

private:
    std::shared_ptr<Detail::CompleterState> state;
};

struct EmptyValue
{
    static void reflect(Reflector&) {}
};

class UnknownCommandError : public std::runtime_error
{
public:
    explicit UnknownCommandError(const std::string& commandToUse);
};

namespace Detail
{

template <typename ReqT, typename ResT>
struct InfoFor
{
    using Req = ReqT;
    using Res = ResT;
    static constexpr bool hasReq = !std::is_void_v<ReqT>;
    static constexpr bool hasRes = !std::is_void_v<ResT>;
};

template <typename T>
concept CallableInfo = requires {
    typename T::Req;
    typename T::Res;
    { T::hasReq } -> std::convertible_to<bool>;
    { T::hasRes } -> std::convertible_to<bool>;
};

template <CallableInfo Info, typename Callable>
std::function<JSON(const JSON&)> makeJsonAdapter(Callable callable)
{
    return [callable = std::move(callable)](const JSON& payload) -> JSON
    {
        ignoreUnused(payload);
        if constexpr (Info::hasReq && Info::hasRes)
        {
            auto req = typename Info::Req {};
            fromJSON(req, Json::payloadOrEmpty(payload));
            return toJSON(callable(req));
        }
        else if constexpr (Info::hasReq && !Info::hasRes)
        {
            auto req = typename Info::Req {};
            fromJSON(req, Json::payloadOrEmpty(payload));
            callable(req);
            return JSON {};
        }
        else if constexpr (!Info::hasReq && Info::hasRes)
        {
            return toJSON(callable());
        }
        else
        {
            callable();
            return JSON {};
        }
    };
}

// The Completer is built before the try so every settlement path — the
// handler's own resolve, a synchronous throw, or all copies being
// dropped — funnels through one single-shot guard.
template <CallableInfo Info, typename Callable>
std::function<void(const JSON&, Resolve)> makeAsyncJsonAdapter(Callable callable)
{
    return [callable = std::move(callable)](const JSON& payload, Resolve resolve)
    {
        auto completer = Completer<typename Info::Res> {std::move(resolve)};

        try
        {
            if constexpr (Info::hasReq)
            {
                auto req = typename Info::Req {};
                fromJSON(req, Json::payloadOrEmpty(payload));
                callable(req, completer);
            }
            else
            {
                ignoreUnused(payload);
                callable(completer);
            }
        }
        catch (const std::exception& e)
        {
            completer.reject(e.what());
        }
    };
}

} // namespace Detail

class CommandTable
{
public:
    using RawHandler = std::function<JSON(const JSON& payload)>;

    template <typename Req, typename Res>
    using TypedHandler = const std::function<Res(const Req&)>;

    template <typename Req, typename Res>
    static RawHandler createRawHandler(const TypedHandler<Req, Res>& handler)
    {
        return Detail::makeJsonAdapter<Detail::InfoFor<Req, Res>>(handler);
    }

    template <typename Req, typename Res>
    void on(const std::string& command, const TypedHandler<Req, Res>& handler)
    {
        registerHandler(command, createRawHandler(handler));
    }

    template <typename Req, typename Res>
    void on(const std::string& command, Res (*handler)(const Req&))
    {
        registerHandler(command,
                        Detail::makeJsonAdapter<Detail::InfoFor<Req, Res>>(handler));
    }

    template <typename Res>
    void on(const std::string& command, Res (*handler)())
    {
        registerHandler(
            command, Detail::makeJsonAdapter<Detail::InfoFor<void, Res>>(handler));
    }

    template <typename Req>
    void on(const std::string& command, void (*handler)(const Req&))
    {
        registerHandler(
            command, Detail::makeJsonAdapter<Detail::InfoFor<Req, void>>(handler));
    }

    void on(const std::string& command, void (*handler)())
    {
        registerHandler(
            command, Detail::makeJsonAdapter<Detail::InfoFor<void, void>>(handler));
    }

    void on(const std::string& command, const RawHandler& handler)
    {
        registerHandler(command, handler);
    }

    // Takes Resolve by value so the handler can settle it long after
    // dispatchAsync has returned, from any thread.
    using AsyncRawHandler =
        std::function<void(const JSON& payload, Resolve resolve)>;

    template <typename Req, typename Res>
    void onAsync(const std::string& command,
                 const std::function<void(const Req&, Completer<Res>)>& handler)
    {
        registerAsyncHandler(
            command,
            Detail::makeAsyncJsonAdapter<Detail::InfoFor<Req, Res>>(handler));
    }

    template <typename Res>
    void onAsync(const std::string& command,
                 const std::function<void(Completer<Res>)>& handler)
    {
        registerAsyncHandler(
            command,
            Detail::makeAsyncJsonAdapter<Detail::InfoFor<void, Res>>(handler));
    }

    void onAsync(const std::string& command, const AsyncRawHandler& handler)
    {
        registerAsyncHandler(command, handler);
    }

    bool has(std::string_view command) const;

    JSON dispatch(std::string_view command, const JSON& payload) const;

    // Never blocks or threads: a sync handler runs inline and any throw
    // becomes the error. Threading is up to whatever the Resolve does.
    void dispatchAsync(std::string_view command,
                       const JSON& payload,
                       const Resolve& resolve) const;

private:
    void registerHandler(const std::string& command, const RawHandler& handler);
    void registerAsyncHandler(const std::string& command,
                              const AsyncRawHandler& handler);

    std::unordered_map<std::string, RawHandler> handlers;
    std::unordered_map<std::string, AsyncRawHandler> asyncHandlers;
};
} // namespace Miro
