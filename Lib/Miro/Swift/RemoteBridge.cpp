#include "RemoteBridge.h"

#include <stdexcept>

namespace Miro::Swift
{

std::function<Miro::JSON(const std::string&, const Miro::JSON&)>
    makeRemoteInvoker(RawDispatchFn dispatchFn, void* ctx, FreeStringFn freeFn)
{
    return [dispatchFn, ctx, freeFn](const std::string& command,
                                     const Miro::JSON& payload) -> Miro::JSON
    {
        auto text = Miro::Json::print(payload);

        auto* error = static_cast<char*>(nullptr);
        auto* result = dispatchFn(ctx, command.c_str(), text.c_str(), &error);

        if (result == nullptr)
        {
            auto message =
                std::string {error != nullptr ? error : "remote dispatch failed"};
            if (error != nullptr)
                freeFn(error);
            throw std::runtime_error(message);
        }

        auto parsed = Miro::Json::parse(result);
        freeFn(result);
        return parsed;
    };
}

} // namespace Miro::Swift
