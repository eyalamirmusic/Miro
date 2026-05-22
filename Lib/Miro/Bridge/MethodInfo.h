#pragma once

// Pmf type extraction for the API-reflection layer.
//
// Given a pointer-to-member-function like &Todos::addTodo, MethodInfo
// pulls out the owning class, the request type (matched against const
// A&), and the response type. The four shapes — Res(const Req&), Res(),
// void(const Req&), void() — each with const and non-const variants
// are all matched. Other shapes (by-value request, non-const reference,
// extra parameters) don't match and produce a compile-time error at
// the use site, which is the intended way to signal a bad handler
// signature.
//
// makePmfHandler<&Class::method>(instance) returns a CommandTable::
// RawHandler closing over &instance and Method, ready to drop into
// CommandTable::on(name, handler). The shape adaptation (req fromJSON,
// res toJSON, void elision on either side) mirrors the existing
// free-function-handler machinery in CommandTable.

#include "../JSON/Json.h"
#include "../Reflection/CommandTable.h"
#include "../Reflection/Serialize.h"

#include <type_traits>

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
};

template <auto Method>
CommandTable::RawHandler
    makePmfHandler(typename MethodInfo<decltype(Method)>::Class& instance)
{
    using Info = MethodInfo<decltype(Method)>;
    auto* instancePtr = &instance;

    return [instancePtr](const JSON& payload) -> JSON
    {
        auto& inst = *instancePtr;

        if constexpr (Info::hasReq && Info::hasRes)
        {
            auto req = typename Info::Req {};
            fromJSON(req, Json::payloadOrEmpty(payload));
            return toJSON((inst.*Method)(req));
        }
        else if constexpr (Info::hasReq && !Info::hasRes)
        {
            auto req = typename Info::Req {};
            fromJSON(req, Json::payloadOrEmpty(payload));
            (inst.*Method)(req);
            return JSON {};
        }
        else if constexpr (!Info::hasReq && Info::hasRes)
        {
            return toJSON((inst.*Method)());
        }
        else
        {
            (inst.*Method)();
            return JSON {};
        }
    };
}

} // namespace Miro
