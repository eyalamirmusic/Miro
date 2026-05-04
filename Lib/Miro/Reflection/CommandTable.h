#pragma once

#include "Reflector.h"
#include "Serialize.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Miro
{
struct EmptyValue
{
    static void reflect(Reflector&) {}
};

class UnknownCommandError : public std::runtime_error
{
public:
    explicit UnknownCommandError(const std::string& commandToUse);
};

class CommandTable
{
public:
    using RawHandler = std::function<Json::Value(const Json::Value& payload)>;

    template <typename Req, typename Res>
    using TypedHandler = const std::function<Res(const Req&)>;

    template <typename Req, typename Res>
    static RawHandler createRawHandler(const TypedHandler<Req, Res>& handler)
    {
        return [handler](const Json::Value& payload) -> Json::Value
        {
            auto req = Req {};
            auto adjusted =
                payload.isNull() ? Json::Value {Json::Object {}} : payload;
            fromJSON(req, adjusted);
            auto res = handler(req);
            return toJSON(res);
        };
    }

    template <typename Req, typename Res>
    void on(const std::string& command, const TypedHandler<Req, Res>& handler)
    {
        registerHandler(command, createRawHandler(handler));
    }

    template <typename Req, typename Res>
    void on(const std::string& command, Res (*handler)(const Req&))
    {
        on<Req, Res>(command, std::function<Res(const Req&)> {handler});
    }

    void on(const std::string& command, const RawHandler& handler)
    {
        registerHandler(command, handler);
    }

    bool has(std::string_view command) const;

    Json::Value dispatch(std::string_view command, const Json::Value& payload) const;

private:
    void registerHandler(const std::string& command, const RawHandler& handler);

    std::unordered_map<std::string, RawHandler> handlers;
};
} // namespace Miro
