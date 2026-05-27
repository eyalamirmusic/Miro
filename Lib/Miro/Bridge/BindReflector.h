#pragma once

// Concrete ApiReflector that wires an API into a live Bridge: each
// `r.command(pmf, name)` installs a RawHandler on the bridge's command
// table; each `r.event(pmd, name)` subscribes a Listener to the event's
// broadcaster that emits the snapshot under `name` whenever the API
// publishes.
//
// Constructed by Bridge::use<Api>(api). The Bridge holds the listener
// pack, so subscriptions die with the bridge.
//
// Bridge is only forward-declared here — overrides are out-of-line in
// BindReflector.cpp where the full type is available, which avoids a
// circular include between Bridge.h and this header.

#include "ApiReflector.h"

#include <ea_data_structures/Pointers/Broadcaster.h>
#include <ea_data_structures/Pointers/OwningPointer.h>

namespace Miro
{

class Bridge;

namespace Detail
{

class BindReflector : public ApiReflector
{
public:
    BindReflector(Bridge& bridgeToUse, void* apiInstanceToUse)
        : ApiReflector(apiInstanceToUse)
        , bridge(bridgeToUse)
    {
    }

    BindReflector(const BindReflector&) = delete;
    BindReflector(BindReflector&&) = delete;
    BindReflector& operator=(const BindReflector&) = delete;
    BindReflector& operator=(BindReflector&&) = delete;

protected:
    void commandImpl(const CommandDescriptor& d) override;
    void eventImpl(const EventDescriptor& d) override;

private:
    Bridge& bridge;
};

} // namespace Detail
} // namespace Miro
