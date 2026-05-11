#pragma once

#include "Types.h"

#include <Miro/Miro.h>

User getUser(const GetUserRequest& req);

ListUsersResponse listUsers();

SendMessageResponse sendMessage(const SendMessageRequest& req);

void logEvent(const LogRequest& req);

void ping();
