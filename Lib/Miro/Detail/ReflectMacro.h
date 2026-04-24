#pragma once

#include "Reflector.h"

// Macro-based reflect() generator.
//
// Usage (intrusive, for types you own):
//   struct Foo
//   {
//       int x = 0;
//       std::string name;
//       MIRO_REFLECT(x, name)
//   };
//
// Usage (non-intrusive, for types you don't own):
//   // at global scope, after Foo is fully declared
//   MIRO_REFLECT_EXTERNAL(Foo, x, name)
//
// Each listed member becomes ref["member"](member). The field name is used
// as the JSON key. Supports up to ~256 fields.
//
// Usage (custom JSON keys, intrusive):
//   MIRO_REFLECT_MEMBERS(x, "X Coord", name, "Full Name")
//
// Usage (custom JSON keys, non-intrusive):
//   MIRO_REFLECT_EXTERNAL_MEMBERS(Foo, x, "X Coord", name, "Full Name")
//
// The ..._MEMBERS variants take (field, keyString) pairs instead of bare
// field names, so the JSON key can differ from the C++ identifier.

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

#define MIRO_REFLECT_FIELD(field) ref[#field](field);

#define MIRO_REFLECT(...)                                                           \
    void reflect([[maybe_unused]] Miro::Reflector& ref)                             \
    {                                                                               \
        MIRO_FOR_EACH(MIRO_REFLECT_FIELD, __VA_ARGS__)                              \
    }

#define MIRO_REFLECT_EXTERNAL_FIELD(field) ref[#field](valueToUse.field);

#define MIRO_REFLECT_EXTERNAL(Type, ...)                                            \
    namespace Miro                                                                  \
    {                                                                               \
    inline void reflect([[maybe_unused]] Miro::Reflector& ref,                      \
                        [[maybe_unused]] Type& valueToUse)                          \
    {                                                                               \
        MIRO_FOR_EACH(MIRO_REFLECT_EXTERNAL_FIELD, __VA_ARGS__)                     \
    }                                                                               \
    }

#define MIRO_REFLECT_NAMED_FIELD(field, key) ref[key](field);

#define MIRO_REFLECT_MEMBERS(...)                                                   \
    void reflect([[maybe_unused]] Miro::Reflector& ref)                             \
    {                                                                               \
        MIRO_FOR_EACH_PAIR(MIRO_REFLECT_NAMED_FIELD, __VA_ARGS__)                   \
    }

#define MIRO_REFLECT_EXTERNAL_NAMED_FIELD(field, key) ref[key](valueToUse.field);

#define MIRO_REFLECT_EXTERNAL_MEMBERS(Type, ...)                                    \
    namespace Miro                                                                  \
    {                                                                               \
    inline void reflect([[maybe_unused]] Miro::Reflector& ref,                      \
                        [[maybe_unused]] Type& valueToUse)                          \
    {                                                                               \
        MIRO_FOR_EACH_PAIR(MIRO_REFLECT_EXTERNAL_NAMED_FIELD, __VA_ARGS__)          \
    }                                                                               \
    }
