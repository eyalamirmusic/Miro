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

void CommandTable::registerAsyncHandler(const std::string& command,
                                        const AsyncRawHandler& handler)
{
    asyncHandlers[command] = handler;
}

bool CommandTable::has(std::string_view command) const
{
    auto key = std::string {command};
    return handlers.contains(key) || asyncHandlers.contains(key);
}

JSON CommandTable::dispatch(std::string_view command, const JSON& payload) const
{
    auto it = handlers.find(std::string {command});

    if (it == handlers.end())
    {
        if (asyncHandlers.contains(std::string {command}))
            throw std::runtime_error("command is async, use dispatchAsync: "
                                     + std::string {command});

        throw UnknownCommandError(std::string {command});
    }

    return it->second(payload);
}

void CommandTable::dispatchAsync(std::string_view command,
                                 const JSON& payload,
                                 const Resolve& resolve) const
{
    auto async = asyncHandlers.find(std::string {command});

    if (async != asyncHandlers.end())
    {
        async->second(payload, resolve);
        return;
    }

    try
    {
        resolve(dispatch(command, payload), nullptr);
    }
    catch (const std::exception& e)
    {
        auto message = std::string {e.what()};
        resolve(JSON {}, &message);
    }
}

} // namespace Miro
