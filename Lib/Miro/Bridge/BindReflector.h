#pragma once

// Bridge is only forward-declared here — the overrides are out-of-line in
// BindReflector.cpp to avoid a circular include with Bridge.h.

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
