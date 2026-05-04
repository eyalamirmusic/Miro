#include "Register.h"

namespace Miro::CommandExport
{

namespace Detail
{

std::vector<CommandEntry>& registry()
{
    static auto entries = std::vector<CommandEntry> {};
    return entries;
}

} // namespace Detail

void registerStaticCommandsInto(CommandTable& table)
{
    for (auto& entry: Detail::registry())
        table.on(entry.name, entry.thunk);
}

} // namespace Miro::CommandExport
