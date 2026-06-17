// Swift integration test app.
//
// Wires the generated Client's `Invoke` closure to the C++ host over the
// CalcHost C ABI, then calls the generated, typed methods and asserts on
// the results. Every call here crosses Swift -> C ABI -> Miro::Bridge ->
// the CalcApi handler in C++ and back, decoding the JSON response into
// the generated Swift types. Exits non-zero if any check fails.

import Foundation

enum HostError: Error { case dispatch(String) }

// Build the transport once: it owns the C++ host for the process and
// translates (command, JSON bytes) <-> the C ABI, including the error
// channel and freeing the C-owned result buffer.
func makeClient() -> (Client, () -> Void) {
    let host = calc_host_create()

    let client = Client(invoke: { command, payload in
        let payloadStr = String(data: payload, encoding: .utf8) ?? "{}"

        var errPtr: UnsafeMutablePointer<CChar>?
        let resultPtr = command.withCString { cmd in
            payloadStr.withCString { pl in
                calc_host_dispatch(host, cmd, pl, &errPtr)
            }
        }

        guard let resultPtr else {
            let message = errPtr.map { String(cString: $0) } ?? "unknown error"
            miro_string_free(errPtr)
            throw HostError.dispatch(message)
        }

        let data = Data(String(cString: resultPtr).utf8)
        miro_string_free(resultPtr)
        return data
    })

    return (client, { calc_host_destroy(host) })
}

var failures = 0
func check(_ condition: Bool, _ message: String) {
    if condition {
        print("✅ \(message)")
    } else {
        print("❌ \(message)")
        failures += 1
    }
}

let (client, teardown) = makeClient()
defer { teardown() }

do {
    // Res(Req): request encoded, response decoded.
    let sum = try client.add(AddRequest(a: 2, b: 3))
    check(sum.result == 5, "add(2, 3).result == 5 (got \(sum.result))")

    // String payloads cross intact.
    let greeting = try client.greet(GreetRequest(name: "Ada"))
    check(greeting.message == "Hello, Ada!",
          "greet(\"Ada\").message == \"Hello, Ada!\" (got \"\(greeting.message)\")")

    // Res(): no request — client sends {}.
    let status = try client.status()
    check(status.ok && status.version == "1.0.0",
          "status() -> ok=\(status.ok), version=\(status.version)")

    // void(): no request, no response — must not throw.
    try client.reset()
    check(true, "reset() completed without throwing")
} catch {
    print("❌ unexpected throw: \(error)")
    failures += 1
}

// Error channel: an unknown command must surface as a thrown error.
do {
    let bogus = Client(invoke: { _, _ in
        var errPtr: UnsafeMutablePointer<CChar>?
        let host = calc_host_create()
        defer { calc_host_destroy(host) }
        let r = "nope".withCString { cmd in
            "{}".withCString { pl in calc_host_dispatch(host, cmd, pl, &errPtr) }
        }
        guard let r else {
            let m = errPtr.map { String(cString: $0) } ?? "unknown error"
            miro_string_free(errPtr)
            throw HostError.dispatch(m)
        }
        miro_string_free(r)
        return Data()
    })
    _ = try bogus.status()
    check(false, "unknown command should have thrown")
} catch {
    check(true, "unknown command throws: \(error)")
}

if failures == 0 {
    print("\nAll integration checks passed.")
    exit(0)
} else {
    print("\n\(failures) check(s) failed.")
    exit(1)
}
