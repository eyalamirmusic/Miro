struct AddRequest: Codable {
    var a: Int
    var b: Int
}

struct AddResponse: Codable {
    var result: Int
}

struct GreetRequest: Codable {
    var name: String
}

struct GreetResponse: Codable {
    var message: String
}

struct StatusResponse: Codable {
    var ok: Bool
    var version: String
}

