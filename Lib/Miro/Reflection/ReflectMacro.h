#pragma once

#include "Reflector.h"

// Recursive expansion machinery: MIRO_PARENS defers each ..._AGAIN so the
// preprocessor rescans it, and the nested MIRO_EXPAND passes bound the
// recursion at roughly 256 items.

#define MIRO_PARENS ()

#define MIRO_EXPAND(...)                                                            \
    MIRO_EXPAND4(MIRO_EXPAND4(MIRO_EXPAND4(MIRO_EXPAND4(__VA_ARGS__))))
#define MIRO_EXPAND4(...)                                                           \
    MIRO_EXPAND3(MIRO_EXPAND3(MIRO_EXPAND3(MIRO_EXPAND3(__VA_ARGS__))))
#define MIRO_EXPAND3(...)                                                           \
    MIRO_EXPAND2(MIRO_EXPAND2(MIRO_EXPAND2(MIRO_EXPAND2(__VA_ARGS__))))
#define MIRO_EXPAND2(...)                                                           \
    MIRO_EXPAND1(MIRO_EXPAND1(MIRO_EXPAND1(MIRO_EXPAND1(__VA_ARGS__))))
#define MIRO_EXPAND1(...) __VA_ARGS__

#define MIRO_FOR_EACH(macro, ...)                                                   \
    __VA_OPT__(MIRO_EXPAND(MIRO_FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define MIRO_FOR_EACH_HELPER(macro, a, ...)                                         \
    macro(a) __VA_OPT__(MIRO_FOR_EACH_AGAIN MIRO_PARENS(macro, __VA_ARGS__))
#define MIRO_FOR_EACH_AGAIN() MIRO_FOR_EACH_HELPER

#define MIRO_FOR_EACH_PAIR(macro, ...)                                              \
    __VA_OPT__(MIRO_EXPAND(MIRO_FOR_EACH_PAIR_HELPER(macro, __VA_ARGS__)))
#define MIRO_FOR_EACH_PAIR_HELPER(macro, a, b, ...)                                 \
    macro(a, b) __VA_OPT__(MIRO_FOR_EACH_PAIR_AGAIN MIRO_PARENS(macro, __VA_ARGS__))
#define MIRO_FOR_EACH_PAIR_AGAIN() MIRO_FOR_EACH_PAIR_HELPER

// Like MIRO_FOR_EACH, but passes `extra` as the first argument of every
// invocation: macro(extra, a) macro(extra, b) ...
#define MIRO_FOR_EACH_WITH(macro, extra, ...)                                       \
    __VA_OPT__(MIRO_EXPAND(MIRO_FOR_EACH_WITH_HELPER(macro, extra, __VA_ARGS__)))
#define MIRO_FOR_EACH_WITH_HELPER(macro, extra, a, ...)                             \
    macro(extra, a)                                                                 \
        __VA_OPT__(MIRO_FOR_EACH_WITH_AGAIN MIRO_PARENS(macro, extra, __VA_ARGS__))
#define MIRO_FOR_EACH_WITH_AGAIN() MIRO_FOR_EACH_WITH_HELPER

#define MIRO_FIELDS_FIELD(refExpr, field) refExpr[#field](field);

// `refExpr` is re-evaluated once per field, so pass a plain reflector name.
#define MIRO_FIELDS(refExpr, ...)                                                   \
    MIRO_FOR_EACH_WITH(MIRO_FIELDS_FIELD, refExpr, __VA_ARGS__)

// The MIRO_API* macros expand to ApiReflector calls; this header does not
// include the bridge layer, so the user must have <Miro/Bridge.h> in scope.
#define MIRO_API_FIELD(refExpr, field) refExpr.use(#field, field);

#define MIRO_API(refExpr, ...)                                                      \
    MIRO_FOR_EACH_WITH(MIRO_API_FIELD, refExpr, __VA_ARGS__)

#define MIRO_REFLECT_API_FIELD(refExpr, field)                                      \
    refExpr.template api<&MiroReflectApiSelf::field>(*this);

#define MIRO_REFLECT_API(...)                                                       \
    void reflect(Miro::ApiReflector& __VA_OPT__(r))                                 \
    {                                                                               \
        __VA_OPT__(                                                                 \
            using MiroReflectApiSelf = std::remove_cvref_t<decltype(*this)>;)       \
        MIRO_FOR_EACH_WITH(MIRO_REFLECT_API_FIELD, r, __VA_ARGS__)                  \
    }

#define MIRO_REFLECT(...)                                                           \
    void reflect(Miro::Reflector& __VA_OPT__(ref))                                  \
    {                                                                               \
        MIRO_FIELDS(ref, __VA_ARGS__)                                               \
    }

#define MIRO_REFLECT_EXTERNAL_FIELD(field) ref[#field](valueToUse.field);

#define MIRO_REFLECT_EXTERNAL(Type, ...)                                            \
    namespace Miro                                                                  \
    {                                                                               \
    inline void reflect(Miro::Reflector& __VA_OPT__(ref),                           \
                        Type& __VA_OPT__(valueToUse))                               \
    {                                                                               \
        MIRO_FOR_EACH(MIRO_REFLECT_EXTERNAL_FIELD, __VA_ARGS__)                     \
    }                                                                               \
    }

#define MIRO_REFLECT_NAMED_FIELD(field, key) ref[key](field);

#define MIRO_REFLECT_MEMBERS(...)                                                   \
    void reflect(Miro::Reflector& __VA_OPT__(ref))                                  \
    {                                                                               \
        MIRO_FOR_EACH_PAIR(MIRO_REFLECT_NAMED_FIELD, __VA_ARGS__)                   \
    }

#define MIRO_REFLECT_EXTERNAL_NAMED_FIELD(field, key) ref[key](valueToUse.field);

#define MIRO_REFLECT_EXTERNAL_MEMBERS(Type, ...)                                    \
    namespace Miro                                                                  \
    {                                                                               \
    inline void reflect(Miro::Reflector& __VA_OPT__(ref),                           \
                        Type& __VA_OPT__(valueToUse))                               \
    {                                                                               \
        MIRO_FOR_EACH_PAIR(MIRO_REFLECT_EXTERNAL_NAMED_FIELD, __VA_ARGS__)          \
    }                                                                               \
    }

// `d` is the dispatcher named by the enclosing MIRO_REFLECT_POLY lambda.
#define MIRO_POLY_ALT_PAIR(type, tag) d.template alt<type>(tag);

#define MIRO_REFLECT_POLY(field, ...)                                               \
    void reflect(Miro::Reflector& ref)                                              \
    {                                                                               \
        Miro::reflectPolymorphic(                                                   \
            ref,                                                                    \
            field,                                                                  \
            [&](auto& __VA_OPT__(d))                                                \
            { MIRO_FOR_EACH_PAIR(MIRO_POLY_ALT_PAIR, __VA_ARGS__) });                \
    }

#define MIRO_REFLECT_EXTERNAL_POLY(Type, field, ...)                                \
    namespace Miro                                                                  \
    {                                                                               \
    inline void reflect(Miro::Reflector& ref, Type& valueToUse)                     \
    {                                                                               \
        reflectPolymorphic(                                                         \
            ref,                                                                    \
            valueToUse.field,                                                       \
            [&](auto& __VA_OPT__(d))                                                \
            { MIRO_FOR_EACH_PAIR(MIRO_POLY_ALT_PAIR, __VA_ARGS__) });                \
    }                                                                               \
    }
