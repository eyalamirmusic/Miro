#pragma once

// C entry points the Swift side exports (swift/Handlers.swift) for this
// example: create/destroy a boxed Handlers instance. Dispatch itself goes
// through the generated, schema-independent miro_swift_dispatch (declared
// in <Miro/Swift/SwiftServerC.h>); these just produce/release the `ctx`.

#ifdef __cplusplus
extern "C"
{
#endif

    void* calc_make_handlers(void);
    void calc_destroy_handlers(void* ctx);

#ifdef __cplusplus
}
#endif
