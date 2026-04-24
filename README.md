# Miro

A lightweight C++20 JSON library with a reflection-based (de)serialization layer.

Miro provides two layers you can use independently:

- **`Miro::Json`** — a `std::variant`-based `Value` type, plus `parse()` and `print()`.
- **`Miro`** — a reflection layer (`toJSON` / `fromJSON`) that serializes your own structs via an intrusive `reflect()` method.

## Requirements

- C++20
- CMake 3.21+

## Building

```bash
cmake -B build
cmake --build build --config Release
```

## Running the tests

Tests are built when Miro is the top-level project (or when `MIRO_BUILD_TESTS=ON`). The test target is `MiroTests` and uses the [NanoTest](https://github.com/eyalamirmusic/NanoTest) framework, fetched automatically.

```bash
ctest --test-dir build --config Release --output-on-failure

# Run a single test by name regex
ctest --test-dir build --config Release -R "Parse object"
```

A benchmark target compares Miro against nlohmann/json and lives in `Tests/Benchmark/`.

## Using Miro in your project

With CMake FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(Miro GIT_REPOSITORY <url> GIT_TAG main)
FetchContent_MakeAvailable(Miro)

target_link_libraries(YourTarget PRIVATE Miro)
```

All functionality is exposed through a single public header: `#include <Miro/Miro.h>`. The library is built as a unity TU (one `.cpp` file) — implementation details live under `Lib/Miro/Detail/` and should not be included directly.

## The `Json` layer

The core type is `Miro::Json::Value`, a variant over `Null`, `bool`, `double`, `std::string`, `Array` (`std::vector<Value>`), and `Object` (`std::map<std::string, Value>`).

```cpp
#include <Miro/Miro.h>

using namespace Miro::Json;

auto value = parse(R"({
    "name": "Miro",
    "version": 1.0,
    "features": ["json", "reflection"],
    "active": true
})");

auto name = value["name"].asString();       // "Miro"
auto first = value["features"][0].asString(); // "json"

auto compact = print(value);       // minified
auto pretty = print(value, 4);     // indented with 4 spaces
```

Accessors (`asBool`, `asNumber`, `asString`, `asArray`, `asObject`) throw `std::bad_variant_access` on type mismatch. Use the `is*` predicates to check first, or use `find(object, key)` to look up an optional value by key.

`ParseError` is thrown on malformed input.

## The reflection layer

Define a `reflect(Miro::Reflector&)` method on your type, binding each field to a key. The same function handles both saving and loading.

```cpp
#include <Miro/Miro.h>

struct Settings
{
    void reflect(Miro::Reflector& ref)
    {
        ref["name"](name);
        ref["count"](count);
        ref["tags"](tags);
    }

    std::string name;
    int count = 0;
    std::vector<std::string> tags;
};

auto s = Settings {"hello", 3, {"a", "b"}};

auto json = Miro::toJSONString(s, 4);
auto loaded = Miro::createFromJSONString<Settings>(json);
```

Built-in reflection is provided for:

- Primitives: `bool`, `int`, `double`, `std::string`
- All other integral types (`unsigned`, `short`, `long`, `long long`, `char`, ...) — serialized as JSON numbers
- `std::vector<T>` and `std::array<T, N>`
- `std::map<std::string, V>`
- `std::optional<T>` — empty optionals serialize as JSON `null`; on load, `null` clears the optional and a missing key leaves it untouched
- Any user type with a `reflect(Reflector&)` method (nested types compose)

Convenience functions:

- `Miro::toJSON(value)` / `Miro::fromJSON(value, json)`
- `Miro::createFromJSON<T>(json)`
- `Miro::toJSONString(value, indent = 0)` / `Miro::fromJSONString(value, str)`
- `Miro::createFromJSONString<T>(str)`

### Reflection macros

Four macros cover the common cases. Pick based on two axes: does your type support adding a member function (intrusive vs. non-intrusive), and do you want the JSON key to match the C++ identifier or to be an arbitrary string?

| | JSON key = field name | JSON key is an explicit string |
| --- | --- | --- |
| **Intrusive** (inside your struct) | `MIRO_REFLECT` | `MIRO_REFLECT_MEMBERS` |
| **Non-intrusive** (for types you don't own) | `MIRO_REFLECT_EXTERNAL` | `MIRO_REFLECT_EXTERNAL_MEMBERS` |

#### `MIRO_REFLECT` — intrusive, field name as key

List the fields inside the struct; each becomes `ref["field"](field)`:

```cpp
struct Settings
{
    std::string name;
    int count = 0;
    std::vector<std::string> tags;

    MIRO_REFLECT(name, count, tags)
};
```

Equivalent to the hand-written `reflect()` method above.

#### `MIRO_REFLECT_MEMBERS` — intrusive, explicit JSON keys

Takes `(field, "key")` pairs when the JSON key needs to differ from the C++ identifier (for example, keys with spaces or hyphens, or keys that are C++ reserved words):

```cpp
struct Product
{
    std::string name;
    double unitPrice = 0.0;

    MIRO_REFLECT_MEMBERS(name, "Full Name", unitPrice, "Unit Price")
};
```

#### `MIRO_REFLECT_EXTERNAL` — non-intrusive, field name as key

For third-party types you can't modify. Place at **file / global scope**, after the type is fully declared, with the fully-qualified type name:

```cpp
namespace ThirdParty { struct Point { int x; int y; }; }

MIRO_REFLECT_EXTERNAL(ThirdParty::Point, x, y)
```

This defines a free-function `reflect` overload in namespace `Miro`, so the type composes with the rest of the reflection layer (nesting, `std::vector`, `std::map`, etc.).

#### `MIRO_REFLECT_EXTERNAL_MEMBERS` — non-intrusive, explicit JSON keys

The external-type variant with custom key strings:

```cpp
MIRO_REFLECT_EXTERNAL_MEMBERS(ThirdParty::Point, x, "X Coord", y, "Y Coord")
```

## License

See `LICENSE.txt`.
