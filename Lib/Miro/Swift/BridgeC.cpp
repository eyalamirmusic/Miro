#include "BridgeC.h"

#include "../Bridge/Bridge.h"
#include "../JSON/Json.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

namespace
{

// Copies `value` into a malloc'd, NUL-terminated buffer the caller owns
// and releases with miro_string_free. Returns nullptr if the allocation
// fails (mirrors what callers already handle for a failed dispatch).
char* dupString(const std::string& value)
{
    auto* out = static_cast<char*>(std::malloc(value.size() + 1));
    if (out != nullptr)
        std::memcpy(out, value.c_str(), value.size() + 1);
    return out;
}

} // namespace

extern "C"
{
    char* miro_bridge_dispatch(MiroBridge* bridge,
                               const char* command,
                               const char* payloadJson,
                               char** errorOut)
    {
        if (errorOut != nullptr)
            *errorOut = nullptr;

        auto* realBridge = reinterpret_cast<Miro::Bridge*>(bridge);

        try
        {
            auto hasPayload = payloadJson != nullptr && payloadJson[0] != '\0';
            auto payload = hasPayload ? Miro::Json::parse(payloadJson)
                                      : Miro::JSON {Miro::Json::Object {}};

            auto result = realBridge->dispatch(command, payload);
            return dupString(Miro::Json::print(result));
        }
        catch (const std::exception& error)
        {
            if (errorOut != nullptr)
                *errorOut = dupString(error.what());
            return nullptr;
        }
        catch (...)
        {
            if (errorOut != nullptr)
                *errorOut = dupString("unknown error");
            return nullptr;
        }
    }

    void miro_string_free(char* str)
    {
        std::free(str);
    }

} // extern "C"
