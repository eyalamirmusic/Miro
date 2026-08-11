#pragma once

#include "../Reflection/CommandTable.h"
#include "../Reflection/TypeName.h"
#include "../TypeTree/TypeTree.h"
#include "Callable.h"

#include <ea_data_structures/Pointers/Broadcaster.h>
#include <ea_data_structures/Pointers/OwningPointer.h>

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Miro
{

class Bridge;
class ApiReflector;

namespace Detail
{

template <typename T>
concept HasApiReflectMember = requires(T& v, ApiReflector& r) { v.reflect(r); };

template <typename T>
concept HasApiExternalReflect = requires(T& v, ApiReflector& r) { reflect(r, v); };

template <typename T>
concept ApiReflectable = HasApiReflectMember<T> || HasApiExternalReflect<T>;

template <typename>
struct PmdTraits;

template <typename C, typename T>
struct PmdTraits<T C::*>
{
    using Class = C;
    using Type = T;
};

} // namespace Detail

// In-place variant of TypeTree::buildTree<T>(): a TypeNode owns its
// children through OwningPointer, so it is built where it will live
// rather than moved into place.
template <typename T>
void buildTreeInto(TypeTree::TypeNode& root)
{
    auto opts = Detail::topLevelOptions<T>(Mode::Save, /*schema=*/true);
    auto reflector = TypeTree::TypeReflector {root, opts};
    auto value = T {};
    Detail::reflectValue(reflector, value);
}

namespace Detail
{
using NodeFunc = std::function<void(TypeTree::TypeNode&)>;

// `buildTree` is empty — and operator bool() false — when the type is
// elided, e.g. a void return or a nullary command.
struct TypeInfo
{
    std::string_view name;
    std::string_view qualifiedName;

    NodeFunc buildTree;

    explicit operator bool() const { return bool(buildTree); }
};

template <typename T>
TypeInfo makeTypeInfo()
{
    return {typeNameOf<T>(),
            qualifiedNameOf<T>(),
            [](TypeTree::TypeNode& root) { buildTreeInto<T>(root); }};
}

struct CommandDescriptor
{
    using HandlerFunc = std::function<CommandTable::RawHandler(void* apiInstance)>;
    using AsyncHandlerFunc =
        std::function<CommandTable::AsyncRawHandler(void* apiInstance)>;

    std::string_view name;

    TypeInfo req;
    TypeInfo res;

    // Exactly one of these is populated: async commands (shaped
    // void(Req, Completer<Res>)) set makeAsyncHandler, the rest makeHandler.
    HandlerFunc makeHandler;
    AsyncHandlerFunc makeAsyncHandler;
};

struct EventDescriptor
{
    using PayloadFunc = std::function<void(const JSON& payload)>;
    using ListenerFunc =
        std::function<OwningPointer<EA::Listener>(void*, PayloadFunc)>;

    std::string_view name;

    TypeInfo payload;

    ListenerFunc makeListener;

    // toJSON of a default-constructed payload: what codegen'd hooks show
    // before the first emit arrives.
    std::function<JSON()> defaultPayloadJson;

    // Populated only by keyedEvent(); codegen emits per-id selector hooks
    // for keyed events instead of one flat store.
    bool isKeyed = false;
    std::string_view collectionField;
    std::string_view keyField;
};
} // namespace Detail

class ApiReflector
{
public:
    virtual ~ApiReflector() = default;

    template <typename Pmf>
        requires std::is_member_function_pointer_v<Pmf>
    void command(Pmf method, std::string_view name)
    {
        using Info = MethodInfo<Pmf>;

        auto fullName = joinedName(name);

        auto d = Detail::CommandDescriptor {};
        d.name = fullName;

        if constexpr (Info::hasReq)
            d.req = Detail::makeTypeInfo<typename Info::Req>();
        if constexpr (Info::hasRes)
            d.res = Detail::makeTypeInfo<typename Info::Res>();

        if constexpr (Info::isAsync)
        {
            d.makeAsyncHandler =
                [method](void* apiInstance) -> CommandTable::AsyncRawHandler
            {
                auto& api = *static_cast<typename Info::Class*>(apiInstance);
                return makeAsyncPmfHandler(method, api);
            };
        }
        else
        {
            d.makeHandler = [method](void* apiInstance) -> CommandTable::RawHandler
            {
                auto& api = *static_cast<typename Info::Class*>(apiInstance);
                return makePmfHandler(method, api);
            };
        }

        commandImpl(d);
    }

    template <auto Pmf>
        requires std::is_member_function_pointer_v<decltype(Pmf)>
    void command()
    {
        command(Pmf, Detail::memberNameOf<Pmf>());
    }

    template <auto Func>
        requires std::is_function_v<std::remove_pointer_t<decltype(Func)>>
    void command()
    {
        using Info = FunctionInfo<decltype(Func)>;

        auto fullName = joinedName(Detail::memberNameOf<Func>());

        auto d = Detail::CommandDescriptor {};
        d.name = fullName;

        if constexpr (Info::hasReq)
            d.req = Detail::makeTypeInfo<typename Info::Req>();
        if constexpr (Info::hasRes)
            d.res = Detail::makeTypeInfo<typename Info::Res>();

        d.makeHandler = [](void*) -> CommandTable::RawHandler
        {
            return Detail::makeJsonAdapter<Info>(
                [](auto&&... args) -> decltype(auto)
                { return Func(std::forward<decltype(args)>(args)...); });
        };

        commandImpl(d);
    }

    // Explicit name only: a lambda type has no source-derived name for
    // memberNameOf to extract.
    template <typename Callable>
        requires(!std::is_member_function_pointer_v<Callable>
                 && !std::is_function_v<std::remove_pointer_t<Callable>>
                 && requires { &Callable::operator(); })
    void command(Callable callable, std::string_view name)
    {
        using Pmf = decltype(&Callable::operator());
        using Info = MethodInfo<Pmf>;

        auto fullName = joinedName(name);

        auto d = Detail::CommandDescriptor {};
        d.name = fullName;

        if constexpr (Info::hasReq)
            d.req = Detail::makeTypeInfo<typename Info::Req>();
        if constexpr (Info::hasRes)
            d.res = Detail::makeTypeInfo<typename Info::Res>();

        if constexpr (Info::isAsync)
        {
            d.makeAsyncHandler = [c = std::move(callable)](
                                     void*) mutable -> CommandTable::AsyncRawHandler
            { return Detail::makeAsyncJsonAdapter<Info>(std::move(c)); };
        }
        else
        {
            d.makeHandler =
                [c = std::move(callable)](void*) mutable -> CommandTable::RawHandler
            { return Detail::makeJsonAdapter<Info>(std::move(c)); };
        }

        commandImpl(d);
    }

    template <auto... Pmfs>
        requires(sizeof...(Pmfs) > 0)
    void commands()
    {
        (command<Pmfs>(), ...);
    }

    template <typename Pmd>
    void event(Pmd member, std::string_view name)
    {
        auto fullName = joinedName(name);
        eventImpl(makeEventDescriptor(member, fullName));
    }

    template <auto Pmd>
    void event()
    {
        event(Pmd, Detail::memberNameOf<Pmd>());
    }

    template <auto... Pmds>
        requires(sizeof...(Pmds) > 0)
    void events()
    {
        (event<Pmds>(), ...);
    }

    // `collectionField` names a vector-typed field on the payload,
    // `keyField` the id-like field on each of its items.
    template <typename Pmd>
    void keyedEvent(Pmd member,
                    std::string_view name,
                    std::string_view collectionField,
                    std::string_view keyField)
    {
        auto fullName = joinedName(name);
        auto d = makeEventDescriptor(member, fullName);
        d.isKeyed = true;
        d.collectionField = collectionField;
        d.keyField = keyField;
        eventImpl(d);
    }

    // `self` is passed explicitly so pmds can be dereferenced without
    // consulting currentApiInstance() — describe mode pushes no frame.
    template <auto... Members, typename Self>
        requires(sizeof...(Members) > 0)
    void api(Self& self)
    {
        (apiOne<Members>(self), ...);
    }

    // Everything the sub declares is emitted under the prefix "key.".
    template <typename Sub>
        requires Detail::ApiReflectable<Sub>
    void use(std::string_view key, Sub& sub)
    {
        frames.add(Frame {static_cast<void*>(&sub), std::string {key}});
        dispatchSubReflect(sub);
        frames.pop_back();
    }

    template <typename Sub>
        requires Detail::ApiReflectable<Sub>
    void use(Sub& sub)
    {
        frames.add(Frame {static_cast<void*>(&sub), std::string {}});
        dispatchSubReflect(sub);
        frames.pop_back();
    }

protected:
    explicit ApiReflector(void* initialInstance = nullptr)
    {
        if (initialInstance != nullptr)
            frames.add(Frame {initialInstance, std::string {}});
    }

    // The instance descriptors bind against: the deepest sub-API walked.
    void* currentApiInstance() const
    {
        return frames.empty() ? nullptr : frames.back().api;
    }

    virtual void commandImpl(const Detail::CommandDescriptor&) = 0;
    virtual void eventImpl(const Detail::EventDescriptor&) = 0;

private:
    struct Frame
    {
        void* api = nullptr;
        std::string prefix;
    };

    template <typename Sub>
    void dispatchSubReflect(Sub& sub)
    {
        if constexpr (Detail::HasApiReflectMember<Sub>)
            sub.reflect(*this);
        else
            reflect(*this, sub);
    }

    template <auto Member, typename Self>
    void apiOne(Self& self)
    {
        using M = decltype(Member);

        if constexpr (std::is_member_function_pointer_v<M>
                      || std::is_function_v<std::remove_pointer_t<M>>)
        {
            command<Member>();
        }
        else
        {
            using Type = typename Detail::PmdTraits<M>::Type;

            if constexpr (EventLike<Type>)
                event<Member>();
            else if constexpr (Detail::ApiReflectable<Type>)
                use(Detail::memberNameOf<Member>(), self.*Member);
            else
                static_assert(sizeof(Type) == 0,
                              "ApiReflector::api: member is neither a "
                              "callable, an Event<T>, nor an "
                              "ApiReflectable sub-API.");
        }
    }

    // Descriptors hold a string_view into the returned string, so callers
    // must keep it alive across commandImpl/eventImpl, which copy the name.
    std::string joinedName(std::string_view local) const
    {
        auto out = std::string {};
        for (auto& f: frames)
        {
            if (f.prefix.empty())
                continue;
            out += f.prefix;
            out += '.';
        }
        out += local;
        return out;
    }

    template <typename Pmd>
    static Detail::EventDescriptor makeEventDescriptor(Pmd member,
                                                       std::string_view name)
    {
        using Info = EventMemberInfo<Pmd>;
        using Payload = typename Info::Payload;

        auto d = Detail::EventDescriptor {};
        d.name = name;
        d.payload = Detail::makeTypeInfo<Payload>();
        d.defaultPayloadJson = [] { return toJSON(Payload {}); };

        d.makeListener = [member](void* apiInstance,
                                  std::function<void(const JSON&)> emit)
            -> OwningPointer<EA::Listener>
        {
            auto& api = *static_cast<typename Info::Class*>(apiInstance);
            auto& event = api.*member;

            return EA::makeOwned<EA::Listener>(
                event.broadcaster(),
                [&event, emit = std::move(emit)] { emit(toJSON(event.snapshot())); },
                EA::Listener::Modes::TriggerOnEvent);
        };

        return d;
    }

    Vector<Frame> frames;
};

} // namespace Miro
