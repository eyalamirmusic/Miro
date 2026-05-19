#include "Bridge.h"

#include "../CommandExport/Register.h"

namespace Miro
{

JSON Bridge::dispatch(std::string_view command, const JSON& payload) const
{
    return commands.dispatch(command, payload);
}

void Bridge::useStaticRegistry()
{
    CommandExport::registerStaticCommandsInto(commands);
}

void Bridge::emitJson(const std::string& eventToUse, const JSON& payloadToUse)
{
    event = eventToUse;
    payload = &payloadToUse;
    onEmit.trigger();
}

} // namespace Miro
