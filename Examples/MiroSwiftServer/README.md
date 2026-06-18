# MiroSwiftServer — C++ → Swift integration example

The reverse of `MiroSwiftExport`: a **C++ app calls into Swift handlers**.
The C++ side drives the generated C++ client (`MiroClient::Client`) over an
invoker that crosses into Swift; the Swift side implements the generated
`Handlers` protocol and the generated `dispatch` routes each call.

```
C++ app ── client.add(req) ──▶ makeRemoteInvoker ──▶ miro_swift_dispatch (C ABI)
                                                            │
                                                            ▼
                                          Swift dispatch(handlers, cmd, payload)
                                                            │
                                                            ▼
                                              CalcHandlers.add(req)  (Swift)
                                                            │
        AddResponse  ◀── fromJSON ◀── JSON bytes ◀──────────┘
```

Both halves are generated from the **same** `CalcApi` (the C++ runtime has a
reflection-driven bridge, but Swift doesn't — so the Swift callee is
generated):

- C++ caller: `cpp-miro` (types) + `cpp-client` (`MiroClient::Client`).
- Swift callee: `swift` (types) + `swift-runtime` + `swift-server`
  (`Handlers` + `dispatch` + the generated `@_cdecl` C-ABI adapter).

## Build & run

CMake's Swift support needs the **Ninja** generator:

```bash
cmake -B build-swift -G Ninja -DMIRO_BUILD_EXAMPLES=ON -DMIRO_UNITY_BUILD=OFF
cmake --build build-swift --target CalcServerDemo
ctest --test-dir build-swift -R CalcServerDemo --output-on-failure
```

Without a Swift compiler the codegen still runs; only the `CalcServerDemo`
target is skipped.

## How it links

- **`CalcSwift`** is a Swift *dynamic* library: generated types + runtime +
  server (with the `@_cdecl` adapter) + the app's `Handlers`. It exports the
  C-ABI symbols (`miro_swift_dispatch`, `calc_make_handlers`, …) — marked
  `public` so they leave the dylib — and self-contains the Swift runtime.
- **`CalcServerDemo`** is the C++ executable. It links `libMiro` + the Swift
  dylib and only resolves those C symbols. `makeRemoteInvoker` wraps
  `miro_swift_dispatch` + the per-app handler context into the `Invoke` the
  generated client expects.

## Layout

| Path | Role |
|------|------|
| `CMakeLists.txt` | Codegen (both sides) + Swift dylib + C++ exe + the CTest. |
| `cpp/CalcApi.h` | The wire contract — single source of truth. C++ bodies are placeholders (never run; codegen needs only the shapes). |
| `cpp/main.cpp` | C++ driver: builds the client over `makeRemoteInvoker` and asserts. |
| `cpp/CalcHandlersC.h` | C decls for the Swift-exported `calc_make_handlers` / `calc_destroy_handlers`. |
| `swift/Handlers.swift` | Implements the generated `Handlers` + the `@_cdecl` factory. |

Generated files are written into the build tree
(`build-swift/Examples/MiroSwiftServer/generated/`), not committed.

## Adding a command

Add a method + `r.command(&CalcApi::foo, "foo")` in `cpp/CalcApi.h`, then
implement `foo` on `CalcHandlers` in Swift. The C++ client method and the
`Handlers` requirement are both generated.
