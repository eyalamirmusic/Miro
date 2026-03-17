# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Miro is a lightweight C++20 JSON parser and printer library. It uses a `std::variant`-based `Value` type supporting null, bool, double, string, Array (`std::vector<Value>`), and Object (`std::vector<std::pair<std::string, Value>>`).

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

The library lives in `Lib/Miro/` with a single public header `Json.h` exposing the `Miro::Json` namespace:
- `Json.h` / `Json.cpp` — `Value` type and accessors
- `Parser.cpp` — JSON parser (internal, exposed via `Miro::Json::parse()`)
- `Printer.cpp` — JSON serializer (internal, exposed via `Miro::Json::print()`)

Tests use the [NanoTest](https://github.com/eyalamirmusic/NanoTest) framework, fetched automatically via CMake FetchContent. Test target is `MiroTests`. Benchmarks compare against nlohmann/json.

## Code Style

- use auto for variables whenever possible
- use modern RAII style
- use explicit return types (not auto) for functions
- Allman brace style, 4-space indentation, 85-column limit, left-aligned pointers
- Enforced via `.clang-format` and `.clang-tidy` in repo root
