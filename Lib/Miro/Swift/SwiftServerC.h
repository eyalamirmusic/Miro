#pragma once

// C declarations for the @_cdecl symbols the generated Swift server
// (`swift-server` format) exports. A C++ caller includes this to drive
// Swift handlers through the Swift dispatch adapter.
//
// The per-app context (a boxed `Handlers` instance) is created on the
// Swift side — the app exposes its own `@_cdecl` factory built on
// miroInstallHandlers / miroReleaseHandlers — and passed back here as the
// opaque `ctx`.

#ifdef __cplusplus
extern "C"
{
#endif

    // Dispatch one command into the Swift handlers identified by `ctx`.
    // Same JSON-in / JSON-out + error + ownership contract as
    // miro_bridge_dispatch, but the callee is Swift: returns owned UTF-8
    // JSON text on success (release with miro_swift_string_free), or NULL
    // and (when errorOut is non-NULL) *errorOut set to an owned message.
    char* miro_swift_dispatch(void* ctx,
                              const char* command,
                              const char* payloadJson,
                              char** errorOut);

    void miro_swift_string_free(char* str);

#ifdef __cplusplus
}
#endif
