#include "EventRegister.h"

namespace Miro::CommandExport::Detail
{

Vector<EventEntry>& eventRegistry()
{
    static auto entries = Vector<EventEntry> {};
    return entries;
}

} // namespace Miro::CommandExport::Detail
