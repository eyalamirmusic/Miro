# MiroSwiftExport — Swift → C++ integration example

A CMake-built, runnable end-to-end demo of the Swift backend: a Swift app
calls typed, generated methods that dispatch into a C++ `Miro::Bridge` and
decode the responses back into generated Swift `Codable` types.

```
Swift app ── Client.add(req) ──▶ Invoke closure ──▶ calc_host_dispatch (C ABI)
                                                          │
                                                          ▼
                                              miro_bridge_dispatch  (libMiro)
                                                          │
                                                          ▼
                                          Miro::Bridge::dispatch → CalcApi::add
                                                          │
        AddResponse  ◀── JSONDecoder ◀── JSON bytes ◀─────┘
```

## Build & run

CMake's Swift support requires the **Ninja** generator (the default Unix
Makefiles generator has none), so the example uses its own build dir:

```bash
cmake -B build-swift -G Ninja -DMIRO_BUILD_EXAMPLES=ON -DMIRO_UNITY_BUILD=OFF
cmake --build build-swift --target CalcDemo
ctest --test-dir build-swift -R CalcDemo --output-on-failure
```

`./run.sh` runs exactly those three steps (and skips cleanly if `swiftc`
or `ninja` is missing). Without a Swift compiler the example still
configures — the C++ host library and the codegen build; only the
`CalcDemo` Swift target is skipped (you'll see a status message).

## How CMake builds it

- **`miro_export(CalcSchema ... FORMATS swift swift-client)`** builds a
  codegen executable from `CalcApi` and emits `generated/Schema.swift` +
  `generated/Schema.client.swift` via the real registered formats.
- **`CalcHost`** is a C++ *shared* library (so the Swift executable only
  resolves the C-ABI symbols, not the C++ runtime).
- **`CalcDemo`** is the Swift executable: `main.swift` + the two generated
  files, with `cpp/CalcHost.h` passed as the `-import-objc-header`
  bridging header, linked against `CalcHost`, and registered as a CTest.

## Layout

| Path | Role |
|------|------|
| `CMakeLists.txt` | Wires codegen + C++ host + Swift app + the CTest. |
| `cpp/CalcApi.h` | The wire contract — **single source of truth**. One `reflect(ApiReflector&)` drives both runtime binding and codegen. |
| `cpp/Host.cpp` | Owns a `Miro::Bridge` bound to a `CalcApi`; exposes it via the `CalcHost` C ABI. |
| `cpp/CalcHost.h` | The C ABI Swift imports (the swiftc **bridging header**). |
| `generated/Schema.swift` | Generated `Codable` types (committed for visibility; regenerated each build). |
| `generated/Schema.client.swift` | Generated typed `Client`. |
| `swift/main.swift` | The integration test: wires the client's `Invoke` to the C ABI and asserts. |

## Adding a command

Add a method + `r.command(&CalcApi::foo, "foo")` in `cpp/CalcApi.h`, then
rebuild. The new method appears on the generated Swift `Client` with no
other edits.

## What this does *not* cover yet

- **C++ → Swift** (C++ calling into Swift handlers) — needs the generated
  Swift dispatch/`Handlers` half.
- **Async** — the client and the C ABI are synchronous; `async throws` +
  completion marshalling across the C ABI is the follow-up.
- **Events** — the one-way host→client event channel isn't wired here.
