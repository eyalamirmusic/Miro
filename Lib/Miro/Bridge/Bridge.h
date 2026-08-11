#pragma once

#include "../Reflection/CommandTable.h"
#include "../Reflection/Serialize.h"
#include "BindReflector.h"

#include <ea_data_structures/Pointers/Broadcaster.h>
#include <ea_data_structures/Pointers/OwningPointer.h>
#include <ea_data_structures/Structures/Vector.h>

#include <functional>
#include <string>
#include <string_view>

namespace Miro
{

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

    // Async handlers own their threading: they may settle the Completer
    // later, from any thread.
    template <typename Req, typename Res>
    void onAsync(const std::string& command,
                 const std::function<void(const Req&, Completer<Res>)>& handler)
    {
        commands.onAsync<Req, Res>(command, handler);
    }

    template <typename Res>
    void onAsync(const std::string& command,
                 const std::function<void(Completer<Res>)>& handler)
    {
        commands.onAsync<Res>(command, handler);
    }

    template <typename T>
    void emit(const std::string& eventToUse, const T& payloadToUse)
    {
        emitJson(eventToUse, toJSON(payloadToUse));
    }

    void emit(const std::string& eventToUse) { emitJson(eventToUse, JSON {}); }

    JSON dispatch(std::string_view command, const JSON& payloadToUse) const;

    // A sync command resolves inline; an async one resolves later.
    void dispatchAsync(std::string_view command,
                       const JSON& payloadToUse,
                       const Resolve& resolve) const;

    // The API must outlive this Bridge: handlers hold &api and listeners
    // point into its broadcasters, which are not nulled out on destruction.
    // Declare the API before the Bridge so it is destroyed last.
    template <typename Api>
    void use(Api& api)
    {
        auto reflector = Detail::BindReflector {*this, &api};
        api.reflect(reflector);
    }

    void emitJson(const std::string& eventToUse, const JSON& payloadToUse);

    void attachListener(OwningPointer<EA::Listener> listener)
    {
        boundListeners.add(std::move(listener));
    }

    CommandTable& commandTable() { return commands; }

    // Carries no arguments: listeners read currentEvent()/currentPayload()
    // from inside their callback.
    EA::Broadcaster onEmit;

    std::string_view currentEvent() const { return event; }
    const JSON& currentPayload() const { return *payload; }

private:
    CommandTable commands;
    std::string_view event;
    const JSON* payload = nullptr;
    Vector<OwningPointer<EA::Listener>> boundListeners;
};

} // namespace Miro
