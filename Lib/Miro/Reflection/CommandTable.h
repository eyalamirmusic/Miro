#pragma once

#include "../IgnoreUnused.h"
#include "Reflector.h"
#include "Serialize.h"

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace Miro
{
// The completion seam between Miro and a host's async runtime. Miro is
// deliberately event-loop-agnostic: dispatchAsync runs the (synchronous)
// command handler and reports the outcome by invoking this std::function
// exactly once — a JSON result (error == nullptr) on success, or a
// non-null error message on failure. A transport hands in a Resolve whose
// body it controls; that's where the host decides threading. Mirrors the
// WebView wire's deliver(id, result, error).
using Resolve = std::function<void(const JSON& result, const std::string* error)>;

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

// One-shot Info struct for cases where the caller already has Req and
// Res as type arguments (CommandTable::on for std::function or
// function-pointer handlers). Distinct from MethodInfo / FunctionInfo
// in Bridge/Callable.h, which derive the same fields from a pmf or
// fn-ptr type. All three satisfy CallableInfo below.
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

// Wraps any callable shaped like the underlying handler (Res(Req),
// Res(), void(Req), void()) into a JSON-in / JSON-out RawHandler.
// Info supplies the four shape flags / types; callable can be a
// function pointer, std::function, lambda, or any other invocable
// matching the implied signature. Used by:
//   - CommandTable::on for std::function + free-fn-ptr handlers
//   - CommandExport::Detail::registerCommand for static-init handlers
//   - makePmfHandler in Bridge/Callable.h for pmf handlers
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

    bool has(std::string_view command) const;

    JSON dispatch(std::string_view command, const JSON& payload) const;

    // Completion-based dispatch: runs the (synchronous) handler and
    // reports the outcome through `resolve` — the result on success, or
    // the message of any thrown exception (including an unknown command)
    // as the error. Miro never blocks or threads here; a transport that
    // wants the call to be asynchronous controls that by where it invokes
    // dispatchAsync and what its Resolve does (see eacp's WebViewBridge).
    void dispatchAsync(std::string_view command,
                       const JSON& payload,
                       const Resolve& resolve) const;

private:
    void registerHandler(const std::string& command, const RawHandler& handler);

    std::unordered_map<std::string, RawHandler> handlers;
};
} // namespace Miro
