#pragma once

#include "Reflector.h"

// Macro-based reflect() generator.
//
// Usage:
//   struct Foo
//   {
//       int x = 0;
//       std::string name;
//       MIRO_REFLECT(x, name)
//   };
//
// Each listed member becomes ref["member"](member). The field name is used
// as the JSON key. Supports up to ~256 fields.

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

#define MIRO_REFLECT_FIELD(field) ref[#field](field);

#define MIRO_REFLECT(...)                                                           \
    void reflect([[maybe_unused]] Miro::Reflector& ref)                             \
    {                                                                               \
        MIRO_FOR_EACH(MIRO_REFLECT_FIELD, __VA_ARGS__)                              \
    }
