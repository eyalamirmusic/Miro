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

void Bridge::emitJson(const std::string& event, const JSON& payload)
{
    for (auto& entry: broadcasters)
        entry.broadcaster(event, payload);
}

Bridge::Subscription Bridge::addBroadcaster(Broadcaster broadcaster)
{
    auto id = ++nextBroadcasterId;
    broadcasters.add(BroadcasterEntry {id, std::move(broadcaster)});
    return Subscription {*this, id};
}

void Bridge::removeBroadcaster(int id)
{
    broadcasters.eraseIf([id](const auto& entry) { return entry.id == id; });
}

Bridge::Subscription::~Subscription()
{
    if (bridge != nullptr && id != 0)
        bridge->removeBroadcaster(id);
}

Bridge::Subscription::Subscription(Subscription&& other) noexcept
    : bridge(other.bridge), id(other.id)
{
    other.bridge = nullptr;
    other.id = 0;
}

Bridge::Subscription& Bridge::Subscription::operator=(Subscription&& other) noexcept
{
    if (this != &other)
    {
        if (bridge != nullptr && id != 0)
            bridge->removeBroadcaster(id);

        bridge = other.bridge;
        id = other.id;
        other.bridge = nullptr;
        other.id = 0;
    }
    return *this;
}

} // namespace Miro
