#include "BindReflector.h"

#include "Bridge.h"

#include <string>
#include <utility>

namespace Miro::Detail
{

void BindReflector::commandImpl(const CommandDescriptor& d)
{
    bridge.commandTable().on(std::string {d.name}, d.makeHandler(apiInstance));
}

void BindReflector::eventImpl(const EventDescriptor& d)
{
    auto emit = [&bridge = bridge, name = std::string {d.name}](
                    const JSON& payload) { bridge.emitJson(name, payload); };

    bridge.attachListener(d.makeListener(apiInstance, std::move(emit)));
}

} // namespace Miro::Detail
