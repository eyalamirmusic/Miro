#include "Bridge.h"

namespace Miro
{

Bridge::Bridge() = default;
Bridge::~Bridge() = default;

void Bridge::attachListener(OwningPointer<EA::Listener> listener)
{
    boundListeners.add(std::move(listener));
}

JSON Bridge::dispatch(std::string_view command, const JSON& payloadToUse) const
{
    return commands.dispatch(command, payloadToUse);
}

void Bridge::dispatchAsync(std::string_view command,
                           const JSON& payloadToUse,
                           const Resolve& resolve) const
{
    commands.dispatchAsync(command, payloadToUse, resolve);
}

void Bridge::emitJson(const std::string& eventToUse, const JSON& payloadToUse)
{
    event = eventToUse;
    payload = &payloadToUse;
    onEmit.trigger();
}

} // namespace Miro
