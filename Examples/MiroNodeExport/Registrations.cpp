// Registers the example types and commands with the Miro type-export
// runner. Compiled into the MiroNodeExport schema library (via SOURCES
// in miro_export) so these static initializers fire when the codegen
// executable starts.

#include "Commands.h"
#include "Types.h"

#include <Miro/CommandExport/Register.h>
#include <Miro/TypeExport/Register.h>

MIRO_EXPORT_TYPES(User,
                  Message,
                  Severity,
                  GetUserRequest,
                  ListUsersResponse,
                  SendMessageRequest,
                  SendMessageResponse,
                  LogRequest)

MIRO_EXPORT_COMMANDS(getUser, listUsers, sendMessage, logEvent, ping)
