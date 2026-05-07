#include "Register.h"

namespace Miro::CommandExport
{

namespace Detail
{

Vector<CommandEntry>& registry()
{
    static auto entries = Vector<CommandEntry> {};
    return entries;
}

} // namespace Detail

void registerStaticCommandsInto(CommandTable& table)
{
    for (auto& entry: Detail::registry())
        table.on(entry.name, entry.thunk);
}

} // namespace Miro::CommandExport
