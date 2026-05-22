#pragma once

// Typed event source for the API-reflection layer.
//
// An Event<T> is the in-class declaration of a push-only event channel.
// It owns the latest payload (`snapshot()`) and an EA::Broadcaster
// listeners attach to. The owning API class calls `publish(T)` after
// each mutation; transports bound to the API via Bridge::use(api)
// subscribe to the broadcaster and forward `snapshot()` over the wire.
//
//   class Todos
//   {
//   public:
//       void addTodo(const AddRequest& r)
//       {
//           state.items.create(...);
//           changes.publish(state);
//       }
//
//       Miro::Event<TodoState> changes;
//   private:
//       TodoState state;
//   };
//
// Non-copyable / non-movable: listeners hold pointers into our
// broadcaster, so silently relocating either side would break
// subscriptions. Wrap in a Pimpl / hold by reference if you need
// to move the owning class around.

#include <ea_data_structures/Pointers/Broadcaster.h>

#include <utility>

namespace Miro
{

template <typename T>
class Event
{
public:
    Event() = default;
    explicit Event(T initial)
        : value(std::move(initial))
    {
    }

    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;

    const T& snapshot() const { return value; }

    EA::Broadcaster& broadcaster() { return bcast; }

    void publish(T next)
    {
        value = std::move(next);
        bcast.trigger();
    }

    void publish() { bcast.trigger(); }

private:
    T value {};
    EA::Broadcaster bcast;
};

} // namespace Miro
