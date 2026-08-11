#pragma once

#include "../Reflection/CommandTable.h"
#include "Event.h"

#include <type_traits>
#include <utility>

namespace Miro
{

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
