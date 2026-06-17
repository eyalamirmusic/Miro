// C++ host: owns a Miro::Bridge bound to a CalcApi and exposes it to
// Swift through the CalcHost C ABI. Dispatch delegates to the library's
// miro_bridge_dispatch (Lib/Miro/Swift/BridgeC.{h,cpp}).

#include "CalcApi.h"
#include "CalcHost.h"

#include <Miro/Miro.h>
#include <Miro/Swift/BridgeC.h>

namespace
{

// Declaration order matters: the Bridge holds handlers bound to `api`
// and must be destroyed first. Members destruct in reverse declaration
// order, so `api` is declared before `bridge` — bridge dies first.
struct Host
{
    CalcApi api;
    Miro::Bridge bridge;

    Host() { bridge.use(api); }
};

} // namespace

extern "C"
{
    CalcHost* calc_host_create(void)
    {
        return reinterpret_cast<CalcHost*>(new Host {});
    }

    void calc_host_destroy(CalcHost* host)
    {
        delete reinterpret_cast<Host*>(host);
    }

    char* calc_host_dispatch(CalcHost* host,
                             const char* command,
                             const char* payloadJson,
                             char** errorOut)
    {
        auto* realHost = reinterpret_cast<Host*>(host);
        auto* bridge = reinterpret_cast<MiroBridge*>(&realHost->bridge);
        return miro_bridge_dispatch(bridge, command, payloadJson, errorOut);
    }
}
