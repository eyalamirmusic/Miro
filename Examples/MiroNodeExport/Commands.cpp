// Stub C++ implementations for the registered commands. This example
// implements the commands on the Node side (see node/src/handlers.ts) —
// the C++ definitions exist only so the codegen executable links. They
// are never executed at runtime: the codegen runner walks the type and
// command registries and writes the generated TS modules without
// invoking any thunk.

#include "Commands.h"

User getUser(const GetUserRequest&)
{
    return User {};
}

ListUsersResponse listUsers()
{
    return ListUsersResponse {};
}

SendMessageResponse sendMessage(const SendMessageRequest&)
{
    return SendMessageResponse {};
}

void logEvent(const LogRequest&) {}

void ping() {}
