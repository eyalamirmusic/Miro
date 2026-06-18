#pragma once

#include <string>

// Static, schema-independent Swift runtime emitted as the `swift-runtime`
// format — the analogue of the TypeScript `bridge` format / BridgeRuntime.ts.
//
// It defines the transport seam the generated Swift client and server bind
// to:
//   - MiroTransport         the outgoing seam (invoke command + JSON bytes,
//                           get JSON bytes back). The C-ABI bridge is one
//                           implementation; HTTP / in-process are others.
//   - MiroClosureTransport  adapts a plain closure to MiroTransport (used by
//                           the generated Client's convenience init).
//   - MiroError / MiroDispatchError  the error types both halves throw.
//
// A project that generates `swift-client` and/or `swift-server` must also
// generate this once into the same module.

namespace Miro::Swift
{

std::string formatRuntime();

} // namespace Miro::Swift
