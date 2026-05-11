#pragma once

#include <Miro/Miro.h>

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
