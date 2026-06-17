#pragma once

// C ABI the Swift app imports (this is the swiftc bridging header). It
// exposes the example's CalcApi-backed host: create a host (which owns a
// Miro::Bridge bound to a CalcApi), dispatch JSON commands against it,
// and tear it down. Dispatch forwards to the library's
// miro_bridge_dispatch, so the JSON-in / JSON-out + error + ownership
// contract is identical to Lib/Miro/Swift/BridgeC.h.

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct CalcHost CalcHost;

    // Allocates a host (CalcApi + Bridge, bound). Never returns NULL on
    // success; pair with calc_host_destroy.
    CalcHost* calc_host_create(void);

    void calc_host_destroy(CalcHost* host);

    // Same contract as miro_bridge_dispatch: returns owned JSON text on
    // success (release with miro_string_free), or NULL + *errorOut on
    // failure.
    char* calc_host_dispatch(CalcHost* host,
                             const char* command,
                             const char* payloadJson,
                             char** errorOut);

    // Releases strings returned by calc_host_dispatch. (Same allocator as
    // the library's miro_string_free.)
    void miro_string_free(char* str);

#ifdef __cplusplus
}
#endif
