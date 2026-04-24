# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Miro is a lightweight C++20 JSON library. It provides two layers:

- `Miro::Json` — a `std::variant`-based `Value` type supporting null, bool, double, string, Array (`std::vector<Value>`), and Object (`std::map<std::string, Value>`), plus `parse()` and `print()` (with optional indent).
- `Miro` — a reflection layer (`Reflector`, `toJSON` / `fromJSON`, `toJSONString` / `fromJSONString`, `createFromJSON[String]`) that serializes user types via an intrusive `reflect(Miro::Reflector&)` method. Built-in support for primitives (`bool`, `int`, `double`, `std::string`), `std::vector<T>`, `std::array<T, N>`, and `std::map<std::string, V>`.

## Build Commands

```bash
# Configure and build
cmake -B build
cmake --build build --config Release

# Run tests
ctest --test-dir build --config Release --output-on-failure

# Run a single test (by name regex)
ctest --test-dir build --config Release -R "TestName"
```

**Always configure with `-DMIRO_UNITY_BUILD=OFF`** so `compile_commands.json` has a per-file entry for each source and the LSP can resolve symbols. `MIRO_UNITY_BUILD` is ON by default (CMake's `UNITY_BUILD` on the `Miro` target) for faster CI/release builds, but it collapses per-file compile commands and breaks LSP navigation.

## Architecture

Each implementation `.cpp` file in `Lib/Miro/Detail/` is a separate translation unit, listed explicitly in `Lib/CMakeLists.txt`.

Public surface:
- `Lib/Miro/Miro.h` — the sole public header. Umbrella that pulls in the Detail headers. User code always includes `<Miro/Miro.h>`.

Implementation (in `Lib/Miro/Detail/`, treat as private):
- `Json.h` / `Json.cpp` — `Miro::Json::Value` type, accessors, `parse()`, `print()`. Also aliases `Miro::JSON = Miro::Json::Value`.
- `Parser.cpp` — JSON parser (exposed via `Miro::Json::parse()`).
- `Printer.cpp` — JSON serializer (exposed via `Miro::Json::print()`).
- `Reflector.h` / `Reflector.cpp` — `Reflector`, `Property`, core `reflect(Reflector&, T&)` dispatch, primitive + keyed overloads.
- `ReflectContainers.h` — `reflect` overloads for `std::vector`, `std::array`, `std::map`.
- `Serialize.h` — `toJSON` / `fromJSON` / `toJSONString` / etc. user-facing helpers.
- `ReflectMacro.h` — `MIRO_REFLECT(field1, field2, ...)` macro that generates a `reflect()` method from a field list.

CMake target is `Miro` (static library). To add a new source file, add it to the `add_library(Miro ...)` list in `Lib/CMakeLists.txt`. When `MIRO_UNITY_BUILD=ON` (the default), CMake batches those sources into a unity TU via the `UNITY_BUILD` target property.

Tests use the [NanoTest](https://github.com/eyalamirmusic/NanoTest) framework, fetched automatically via CMake FetchContent. Test target is `MiroTests`. Benchmarks live in `Tests/Benchmark/` and compare against nlohmann/json. Both NanoTest and nlohmann/json are fetched in `Tests/CMakeLists.txt` (and `Tests/Benchmark/CMakeLists.txt` respectively); the root `CMakeLists.txt` only does `enable_testing()` + `add_subdirectory(Tests)` when the project is top-level or `MIRO_BUILD_TESTS=ON`.

## Code Style

- use auto for variables whenever possible
- use modern RAII style
- use explicit return types (not auto) for functions
- Allman brace style, 4-space indentation, 85-column limit, left-aligned pointers
- Enforced via `.clang-format` and `.clang-tidy` in repo root
- Use clang-format after every change in a source file or header file
- struct/class members go on last, below methods
- do not use m_ or _ prefixes. Instead use xToUse for input variables
