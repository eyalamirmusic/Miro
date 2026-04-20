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

## Architecture

The library lives in `Lib/Miro/`. Public headers:
- `Json.h` — `Miro::Json::Value` type, accessors, `parse()`, `print()`. Also aliases `Miro::JSON = Miro::Json::Value`.
- `Miro.h` — reflection layer built on top of `Json.h` (`Reflector`, `Property`, `reflect()` overloads, `toJSON` / `fromJSON` helpers).

Implementation files:
- `Json.cpp` — `Value` constructors and accessors
- `Parser.cpp` — JSON parser (exposed via `Miro::Json::parse()`)
- `Printer.cpp` — JSON serializer (exposed via `Miro::Json::print()`)

Tests use the [NanoTest](https://github.com/eyalamirmusic/NanoTest) framework, fetched automatically via CMake FetchContent. Test target is `MiroTests`. Benchmarks compare against nlohmann/json.

## Code Style

- use auto for variables whenever possible
- use modern RAII style
- use explicit return types (not auto) for functions
- Allman brace style, 4-space indentation, 85-column limit, left-aligned pointers
- Enforced via `.clang-format` and `.clang-tidy` in repo root
- Use clang-format after every change in a source file or header file
- struct/class members go on last, below methods
- do not use m_ or _ prefixes. Instead use xToUse for input variables
