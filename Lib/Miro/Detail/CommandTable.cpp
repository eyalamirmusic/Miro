#include "CommandTable.h"

namespace Miro
{

UnknownCommandError::UnknownCommandError(const std::string& commandToUse)
    : std::runtime_error("unknown command: " + commandToUse)
{
}

void CommandTable::registerHandler(const std::string& command,
                                   const RawHandler& handler)
{
    handlers[command] = handler;
}

bool CommandTable::has(std::string_view command) const
{
    return handlers.contains(std::string {command});
}

Json::Value CommandTable::dispatch(std::string_view command,
                                   const Json::Value& payload) const
{
    auto it = handlers.find(std::string {command});

    if (it == handlers.end())
        throw UnknownCommandError(std::string {command});

    return it->second(payload);
}

} // namespace Miro
