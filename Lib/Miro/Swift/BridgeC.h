#pragma once

// C ABI over Miro::Bridge for Swift (and any other C-FFI) clients.
//
// This is the transport seam for the Swift -> C++ direction: a generated
// Swift `Client`'s `Invoke` closure forwards JSON bytes here, the call
// lands in Bridge::dispatch synchronously, and JSON bytes come back. The
// header is deliberately plain C so it can be imported directly by Swift
// (via a bridging header or module map) with no C++ on the import path.
//
// MiroBridge is an opaque handle that is really a Miro::Bridge*; a C++
// host obtains one with `reinterpret_cast<MiroBridge*>(&bridge)` after
// binding its API (bridge.use(api)). The Bridge must outlive every
// dispatch call made against it.

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct MiroBridge MiroBridge;

    // Dispatch one command synchronously. `command` and `payloadJson` are
    // NUL-terminated UTF-8; `payloadJson` is JSON text and may be NULL or
    // empty, which is treated as an empty object ("{}").
    //
    // On success returns newly-allocated UTF-8 JSON text that the caller
    // owns and must release with miro_string_free; *errorOut (when errorOut
    // is non-NULL) is left NULL.
    //
    // On failure (unknown command, handler threw, malformed payload) returns
    // NULL and, when errorOut is non-NULL, sets *errorOut to a newly-
    // allocated message the caller must also release with miro_string_free.
    char* miro_bridge_dispatch(MiroBridge* bridge,
                               const char* command,
                               const char* payloadJson,
                               char** errorOut);

    // Releases a string returned by miro_bridge_dispatch (either the result
    // or an *errorOut message). Safe to call with NULL.
    void miro_string_free(char* str);

#ifdef __cplusplus
}
#endif
