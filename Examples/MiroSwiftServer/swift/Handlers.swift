// The Swift callee: a concrete implementation of the generated `Handlers`
// protocol, plus the per-app @_cdecl factory that boxes it into the opaque
// context the C++ side passes back to miro_swift_dispatch.
//
// Everything else — the Handlers protocol, dispatch(), the box, and the
// miro_swift_dispatch / miro_swift_string_free entry points — is generated
// (Schema.server.swift) by the `swift-server` format.

import Foundation

struct CalcHandlers: Handlers {
    func add(_ req: AddRequest) throws -> AddResponse {
        AddResponse(result: req.a + req.b)
    }

    func greet(_ req: GreetRequest) throws -> GreetResponse {
        GreetResponse(message: "Hello, \(req.name)!")
    }

    func status() throws -> StatusResponse {
        StatusResponse(ok: true, version: "1.0.0")
    }

    func reset() throws {
        // no-op for the demo
    }
}

// `public` so these are exported from the dynamic library for C++ to link.
@_cdecl("calc_make_handlers")
public func calcMakeHandlers() -> UnsafeMutableRawPointer {
    miroInstallHandlers(CalcHandlers())
}

@_cdecl("calc_destroy_handlers")
public func calcDestroyHandlers(_ ctx: UnsafeMutableRawPointer) {
    miroReleaseHandlers(ctx)
}
