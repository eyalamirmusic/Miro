// Typed client: one method per command over an injected JSON transport.
import Foundation

final class Client {
    typealias Invoke = (_ command: String, _ payload: Data) throws -> Data

    private let invoke: Invoke
    private let encoder = JSONEncoder()
    private let decoder = JSONDecoder()

    init(invoke: @escaping Invoke) {
        self.invoke = invoke
    }

    func add(_ req: AddRequest) throws -> AddResponse {
        let payload = try encoder.encode(req)
        let result = try invoke("add", payload)
        return try decoder.decode(AddResponse.self, from: result)
    }

    func greet(_ req: GreetRequest) throws -> GreetResponse {
        let payload = try encoder.encode(req)
        let result = try invoke("greet", payload)
        return try decoder.decode(GreetResponse.self, from: result)
    }

    func status() throws -> StatusResponse {
        let payload = Data("{}".utf8)
        let result = try invoke("status", payload)
        return try decoder.decode(StatusResponse.self, from: result)
    }

    func reset() throws {
        let payload = Data("{}".utf8)
        _ = try invoke("reset", payload)
    }
}
