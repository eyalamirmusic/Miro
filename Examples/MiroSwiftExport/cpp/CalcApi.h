#pragma once

// The single source of truth for this example's wire contract. The same
// reflect(ApiReflector&) body drives BOTH:
//   - runtime binding on the C++ host  (bridge.use(api))
//   - Swift code generation             (Codegen.cpp walks it via
//     DescribeReflector and emits Schema.swift + Schema.client.swift)
//
// Add a command here and it shows up on the generated Swift client with
// no other edits.

#include <Miro/Miro.h>

#include <string>

struct AddRequest
{
    int a = 0;
    int b = 0;

    MIRO_REFLECT(a, b)
};

struct AddResponse
{
    int result = 0;

    MIRO_REFLECT(result)
};

struct GreetRequest
{
    std::string name;

    MIRO_REFLECT(name)
};

struct GreetResponse
{
    std::string message;

    MIRO_REFLECT(message)
};

struct StatusResponse
{
    bool ok = true;
    std::string version;

    MIRO_REFLECT(ok, version)
};

class CalcApi
{
public:
    void reflect(Miro::ApiReflector& r)
    {
        r.command(&CalcApi::add, "add");
        r.command(&CalcApi::greet, "greet");
        r.command(&CalcApi::status, "status");
        r.command(&CalcApi::reset, "reset");
    }

    AddResponse add(const AddRequest& req) { return {req.a + req.b}; }

    GreetResponse greet(const GreetRequest& req)
    {
        return {"Hello, " + req.name + "!"};
    }

    StatusResponse status() const { return {true, "1.0.0"}; }

    void reset() { resetCount++; }

    int resetCount = 0;
};
