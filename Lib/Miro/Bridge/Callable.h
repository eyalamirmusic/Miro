#pragma once

// Callable inspection traits + the pmf-to-RawHandler factories that
// use them. Both traits and the JSON shape-adapter live next to where
// they're consumed (the adapter is in Reflection/CommandTable.h, since
// RawHandler is defined there). This header pulls in the traits used
// across the codebase plus the makePmfHandler convenience factory.
//
//   MethodInfo<Pmf>      — pointer-to-member-function info (Class, Req,
//                          Res, hasReq, hasRes, isConst). Specialised
//                          for Res(const Req&) / Res() / void(const
//                          Req&) / void(), each in const and non-const
//                          flavours.
//
//   FunctionInfo<F*>     — free-function-pointer info (Req, Res,
//                          hasReq, hasRes). Same shape coverage.
//
// Both satisfy Detail::CallableInfo, so they plug into
// Detail::makeJsonAdapter<Info>(callable) without further adapters.

#include "../Reflection/CommandTable.h"
#include "Event.h"

#include <type_traits>
#include <utility>

namespace Miro
{

// ---------- Pointer-to-member-function (pmf) traits ----------

template <typename T>
struct MethodInfo;

template <typename C, typename R, typename A>
struct MethodInfo<R (C::*)(const A&)>
{
    using Class = C;
    using Req = A;
    using Res = R;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = false;
    static constexpr bool isAsync = false;
};

template <typename C, typename R, typename A>
struct MethodInfo<R (C::*)(const A&) const>
{
    using Class = C;
    using Req = A;
    using Res = R;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = true;
    static constexpr bool isAsync = false;
};

template <typename C, typename R>
struct MethodInfo<R (C::*)()>
{
    using Class = C;
    using Req = void;
    using Res = R;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = false;
    static constexpr bool isAsync = false;
};

template <typename C, typename R>
struct MethodInfo<R (C::*)() const>
{
    using Class = C;
    using Req = void;
    using Res = R;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = true;
    static constexpr bool isAsync = false;
};

// ---------- Async (Completer) pmf traits ----------
//
// A continuation-style handler `void(const Req&, Completer<Res>)` (or the
// no-request `void(Completer<Res>)`) — derives the same Req / Res / hasReq
// / hasRes fields as the synchronous shapes so it plugs into the shared
// adapters, and flags isAsync so ApiReflector routes it through onAsync.
// Res is recovered from the Completer's type argument (void → no response).

template <typename C, typename A, typename R>
struct MethodInfo<void (C::*)(const A&, Completer<R>)>
{
    using Class = C;
    using Req = A;
    using Res = R;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = false;
    static constexpr bool isAsync = true;
};

template <typename C, typename A, typename R>
struct MethodInfo<void (C::*)(const A&, Completer<R>) const>
{
    using Class = C;
    using Req = A;
    using Res = R;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = true;
    static constexpr bool isAsync = true;
};

template <typename C, typename R>
struct MethodInfo<void (C::*)(Completer<R>)>
{
    using Class = C;
    using Req = void;
    using Res = R;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = false;
    static constexpr bool isAsync = true;
};

template <typename C, typename R>
struct MethodInfo<void (C::*)(Completer<R>) const>
{
    using Class = C;
    using Req = void;
    using Res = R;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isConst = true;
    static constexpr bool isAsync = true;
};

// ---------- Event-member pointer traits ----------
//
// For a pointer-to-data-member like &Todos::changes whose type is
// Event<TodoState> Class::*, extracts the owning class and the
// payload type. Used by ApiReflector::event(pmd, name) to derive
// the subscription wiring and the codegen-side payload identity.

template <typename T>
struct EventMemberInfo;

template <typename C, typename T>
struct EventMemberInfo<Event<T> C::*>
{
    using Class = C;
    using Payload = T;
};

template <typename C, typename T>
struct EventMemberInfo<RefEvent<T> C::*>
{
    using Class = C;
    using Payload = T;
};

// ---------- Free-function-pointer traits ----------

template <typename T>
struct FunctionInfo;

template <typename R, typename A>
struct FunctionInfo<R (*)(const A&)>
{
    using Req = A;
    using Res = R;
    static constexpr bool hasReq = true;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isAsync = false;
};

template <typename R>
struct FunctionInfo<R (*)()>
{
    using Req = void;
    using Res = R;
    static constexpr bool hasReq = false;
    static constexpr bool hasRes = !std::is_void_v<R>;
    static constexpr bool isAsync = false;
};

// ---------- makePmfHandler — pmf-to-RawHandler ----------
//
// Two overloads, identical in behaviour:
//   makePmfHandler<&Class::method>(instance)    template-arg form
//   makePmfHandler(&Class::method, instance)    value-arg form
//
// The value-arg form is what ApiReflector::command(pmf, name) calls —
// it accepts the pmf as a regular value so the user's reflect() body
// can pass `&Class::method` directly without `<>`, matching how
// Miro::Reflector takes member references as values.
//
// Body of both is one line: a generic lambda that forwards args
// through the pmf, wrapped in the shared makeJsonAdapter. The if-
// constexpr ladder over hasReq/hasRes lives in makeJsonAdapter only.

template <auto Method>
CommandTable::RawHandler
    makePmfHandler(typename MethodInfo<decltype(Method)>::Class& instance)
{
    using Info = MethodInfo<decltype(Method)>;
    auto* instancePtr = &instance;
    return Detail::makeJsonAdapter<Info>(
        [instancePtr](auto&&... args) -> decltype(auto)
        { return ((*instancePtr).*Method)(std::forward<decltype(args)>(args)...); });
}

template <typename Pmf>
CommandTable::RawHandler makePmfHandler(Pmf method,
                                        typename MethodInfo<Pmf>::Class& instance)
{
    using Info = MethodInfo<Pmf>;
    auto* instancePtr = &instance;
    return Detail::makeJsonAdapter<Info>(
        [method, instancePtr](auto&&... args) -> decltype(auto)
        { return ((*instancePtr).*method)(std::forward<decltype(args)>(args)...); });
}

// Async sibling of makePmfHandler: binds an async pmf (void(const Req&,
// Completer<Res>) / void(Completer<Res>)) to an instance and wraps it as
// an AsyncRawHandler via makeAsyncJsonAdapter. The handler returns void —
// the Completer carries the result out — so no decltype(auto) here.
template <typename Pmf>
CommandTable::AsyncRawHandler
    makeAsyncPmfHandler(Pmf method, typename MethodInfo<Pmf>::Class& instance)
{
    using Info = MethodInfo<Pmf>;
    auto* instancePtr = &instance;
    return Detail::makeAsyncJsonAdapter<Info>(
        [method, instancePtr](auto&&... args)
        { ((*instancePtr).*method)(std::forward<decltype(args)>(args)...); });
}

} // namespace Miro
