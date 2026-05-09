#pragma once

#include "../Containers.h"
#include "../Reflection/CommandTable.h"
#include "../Reflection/Serialize.h"

#include <functional>
#include <string>
#include <string_view>

namespace Miro
{

// Bridge is the runtime primitive transports plug into. It owns a
// CommandTable for incoming requests and a list of Broadcasters for
// outgoing events; transports (eacp WebView, HTTP RPC, WebSocket, ...)
// are thin adapters that route their wire format through dispatch()
// and addBroadcaster().
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

    using Broadcaster =
        std::function<void(std::string_view event, const JSON& payload)>;

    // RAII handle returned by addBroadcaster. Removes the broadcaster
    // from the bridge in its destructor; callers typically store one
    // as a member of the transport adapter so the broadcaster lives
    // exactly as long as the transport.
    class Subscription
    {
    public:
        Subscription() = default;
        ~Subscription();

        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        Subscription(Subscription&& other) noexcept;
        Subscription& operator=(Subscription&& other) noexcept;

    private:
        friend class Bridge;
        Subscription(Bridge& bridgeToUse, int idToUse)
            : bridge(&bridgeToUse), id(idToUse) {}

        Bridge* bridge = nullptr;
        int id = 0;
    };

    Subscription addBroadcaster(Broadcaster broadcaster);

private:
    void emitJson(const std::string& event, const JSON& payload);
    void removeBroadcaster(int id);

    struct BroadcasterEntry
    {
        int id = 0;
        Broadcaster broadcaster;
    };

    CommandTable commands;
    Vector<BroadcasterEntry> broadcasters;
    int nextBroadcasterId = 0;
};

} // namespace Miro
