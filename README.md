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

## Exporting types to other languages

Once your types are reflected, Miro can generate matching definitions for other languages and tools. The supported formats are:

| Format       | Output extension | What it produces                                  |
| ------------ | ---------------- | ------------------------------------------------- |
| `ts`         | `.ts`            | TypeScript interfaces                             |
| `zod`        | `.zod.ts`        | Zod schemas (TypeScript runtime validators)       |
| `jsonschema` | `.schema.json`   | JSON Schema (Draft 2020-12)                       |
| `cpp`        | `.types.h`       | Plain C++ structs (no Miro dependency)            |
| `cpp-miro`   | `.miro.h`        | C++ structs with `MIRO_REFLECT(...)` baked in     |

The pipeline is wired through CMake: a small helper builds a tiny executable that links your reflected types, runs it as a post-build step, and writes one bundled file per requested format.

### Setting up an export target

```cmake
miro_add_type_export(
    NAME       MySchema
    SOURCES    Registrations.cpp
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
    FORMATS    ts zod jsonschema cpp cpp-miro   # optional; defaults to all
)
```

The `miro_add_type_export()` function is registered by Miro's top-level `CMakeLists.txt`, so it is available anywhere downstream of `FetchContent_MakeAvailable(Miro)` — no extra `include()` needed.

Arguments:

- **`NAME`** (required) — name of the executable target. Also the default output basename.
- **`SOURCES`** — `.cpp` files (relative to the caller's `CMakeLists.txt`) that contain `MIRO_EXPORT_TYPES(...)` registrations. Compiled directly into the executable.
- **`LIBRARIES`** — extra libraries to link. Static libraries are linked with `WHOLE_ARCHIVE` so the registrations' static initializers survive linking; `INTERFACE` libraries are linked plainly.
- **`OUTPUT_DIR`** (required) — directory the generated files are written to.
- **`OUTPUT_NAME`** — output filename stem (defaults to `NAME`).
- **`FORMATS`** — subset of the table above (defaults to all).

At least one of `SOURCES` or `LIBRARIES` must be provided.

### Registering types

In one of the `SOURCES` files (or in a library passed via `LIBRARIES`), include `<Miro/TypeExport/Register.h>` and list the reflected types to export:

```cpp
#include "Types.h"

#include <Miro/TypeExport/Register.h>

MIRO_EXPORT_TYPES(User, Address, Role)
```

Every listed type must already have reflection — either an intrusive `reflect()` method, one of the `MIRO_REFLECT*` macros, or an `MIRO_REFLECT_EXTERNAL*` declaration. Enums work too (declared via `MIRO_REFLECT_ENUM(...)`).

### What happens at build time

Each time the export target is rebuilt, CMake runs the executable as a `POST_BUILD` step. The runner walks the registry once per requested format and writes one bundled file per format to `OUTPUT_DIR`, named `<OUTPUT_NAME>.<extension>`.

With `NAME=MySchema` and the default formats, the output directory ends up with:

```
MySchema.ts
MySchema.zod.ts
MySchema.schema.json
MySchema.types.h
MySchema.miro.h
```

You can then `target_sources()` or otherwise consume those files from another target — they regenerate automatically whenever the reflected types change.

### A complete example

`Examples/MiroTypesExport/` is a working end-to-end example: three reflected types (`User`, `Address`, `Role`) wired up through `miro_add_type_export(...)`. Build with `-DMIRO_BUILD_EXAMPLES=ON` (or as the top-level project) and inspect the generated files under `build/Examples/MiroTypesExport/`.

## License

See `LICENSE.txt`.
