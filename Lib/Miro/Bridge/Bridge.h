#pragma once

#include "../Reflection/CommandTable.h"
#include "../Reflection/Serialize.h"

#include <ea_data_structures/Pointers/Broadcaster.h>

#include <functional>
#include <string>
#include <string_view>

namespace Miro
{

// Bridge is the runtime primitive transports plug into. It owns a
// CommandTable for incoming requests and an EA::Broadcaster (onEmit)
// for outgoing events; transports (eacp WebView, HTTP RPC, WebSocket,
// ...) are thin adapters that route their wire format through
// dispatch() and an EA::Listener attached to onEmit.
//
// One Bridge can serve multiple transports simultaneously — a typical
// app declares the Bridge once, calls useStaticRegistry() to pull in
// MIRO_EXPORT_COMMAND-registered handlers, and hands a reference to
// each transport's constructor.
class Bridge
{
public:
    Bridge() = default;
    ~Bridge() = default;

    Bridge(const Bridge&) = delete;
    Bridge& operator=(const Bridge&) = delete;
    Bridge(Bridge&&) = delete;
    Bridge& operator=(Bridge&&) = delete;

    template <typename Req, typename Res>
    void on(const std::string& command,
            const std::function<Res(const Req&)>& handler)
    {
        commands.on<Req, Res>(command, handler);
    }

    template <typename Req, typename Res>
    void on(const std::string& command, Res (*handler)(const Req&))
    {
        commands.on<Req, Res>(command, handler);
    }

    template <typename Res>
    void on(const std::string& command, Res (*handler)())
    {
        commands.on(command, handler);
    }

    template <typename Req>
    void on(const std::string& command, void (*handler)(const Req&))
    {
        commands.on(command, handler);
    }

    void on(const std::string& command, void (*handler)())
    {
        commands.on(command, handler);
    }

    template <typename T>
    void emit(const std::string& event, const T& payload)
    {
        emitJson(event, toJSON(payload));
    }

    void emit(const std::string& event) { emitJson(event, JSON {}); }

    JSON dispatch(std::string_view command, const JSON& payload) const;

    void useStaticRegistry();

    CommandTable& commandTable() { return commands; }

    // Fires on every emit. Transports attach an EA::Listener and read
    // the current event/payload via currentEvent()/currentPayload()
    // inside their callback.
    EA::Broadcaster onEmit;

    std::string_view currentEvent() const { return event; }
    const JSON& currentPayload() const { return *payload; }

private:
    void emitJson(const std::string& eventToUse, const JSON& payloadToUse);

    CommandTable commands;
    std::string_view event;
    const JSON* payload = nullptr;
};

} // namespace Miro
