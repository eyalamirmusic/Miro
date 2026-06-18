#pragma once

// Wire contract for the C++ -> Swift example. The same reflect() body
// drives codegen for BOTH sides: the C++ caller (cpp-miro + cpp-client)
// and the Swift callee (swift + swift-runtime + swift-server). Add a
// command here and it shows up on both generated surfaces.

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

// The real handlers live in Swift (swift/Handlers.swift). The bodies here
// are placeholders that are never executed — they exist only so the
// codegen tool, which takes the address of each command method, links.
// Codegen reads the method *shapes*; nothing on the C++ side dispatches.
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

    AddResponse add(const AddRequest&) { return {}; }
    GreetResponse greet(const GreetRequest&) { return {}; }
    StatusResponse status() const { return {}; }
    void reset() {}
};
