#include "BindReflector.h"

#include "Bridge.h"

#include <string>
#include <utility>

namespace Miro::Detail
{

void BindReflector::commandImpl(const CommandDescriptor& d)
{
    if (d.makeAsyncHandler)
        bridge.commandTable().onAsync(std::string {d.name},
                                      d.makeAsyncHandler(currentApiInstance()));
    else
        bridge.commandTable().on(std::string {d.name},
                                 d.makeHandler(currentApiInstance()));
}

void BindReflector::eventImpl(const EventDescriptor& d)
{
    auto emit = [&bridge = bridge, name = std::string {d.name}](const JSON& payload)
    { bridge.emitJson(name, payload); };

    bridge.attachListener(d.makeListener(currentApiInstance(), std::move(emit)));
}

} // namespace Miro::Detail
