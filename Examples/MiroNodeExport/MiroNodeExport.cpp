// Single-file example. Declares the domain types, defines stub
// command bodies (never executed — the codegen runner only walks the
// registry to emit code), and registers everything with the type and
// command export macros.

#include <Miro/CommandExport/Register.h>
#include <Miro/Miro.h>
#include <Miro/TypeExport/Register.h>

#include <string>
#include <vector>

enum class Severity
{
    Info,
    Warning,
    Error
};

struct User
{
    std::string id;
    std::string name;
    bool isAdmin = false;

    MIRO_REFLECT(id, name, isAdmin)
};

struct Message
{
    std::string id;
    std::string fromUserId;
    std::string text;
    double timestamp = 0.0;

    MIRO_REFLECT(id, fromUserId, text, timestamp)
};

struct GetUserRequest
{
    std::string id;

    MIRO_REFLECT(id)
};

struct ListUsersResponse
{
    std::vector<User> users;

    MIRO_REFLECT(users)
};

struct SendMessageRequest
{
    std::string fromUserId;
    std::string text;

    MIRO_REFLECT(fromUserId, text)
};

struct SendMessageResponse
{
    Message message;

    MIRO_REFLECT(message)
};

struct LogRequest
{
    Severity severity = Severity::Info;
    std::string message;

    MIRO_REFLECT(severity, message)
};

User getUser(const GetUserRequest&)
{
    return {};
}

ListUsersResponse listUsers()
{
    return {};
}

SendMessageResponse sendMessage(const SendMessageRequest&)
{
    return {};
}

void logEvent(const LogRequest&) {}

void ping() {}

MIRO_EXPORT_TYPES(User,
                  Message,
                  Severity,
                  GetUserRequest,
                  ListUsersResponse,
                  SendMessageRequest,
                  SendMessageResponse,
                  LogRequest)

MIRO_EXPORT_COMMANDS(getUser, listUsers, sendMessage, logEvent, ping)
