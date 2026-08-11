#pragma once

#include "../Containers.h"
#include "ReflectDispatch.h"
#include "Reflector.h"
#include "TypeName.h"

#include <string_view>
#include <variant>

// Wire format is externally tagged: an object with exactly one key, the
// tag of the active alternative, whose value is that alternative's body.

namespace Miro::Detail
{

// Customization point for polymorphic storage — specialize it for new
// holder types; the specializations below spell out the contract.
template <typename Value>
struct PolymorphicAccess;

template <typename... Ts>
struct PolymorphicAccess<std::variant<Ts...>>
{
    template <typename T>
    static bool isActive(const std::variant<Ts...>& value)
    {
        return std::holds_alternative<T>(value);
    }

    template <typename T>
    static T& get(std::variant<Ts...>& value)
    {
        return std::get<T>(value);
    }

    template <typename T>
    static T& assign(std::variant<Ts...>& value)
    {
        return value.template emplace<T>();
    }
};

template <typename Base>
struct PolymorphicAccess<OwningPointer<Base>>
{
    template <typename T>
    static bool isActive(const OwningPointer<Base>& value)
    {
        return value.get() != nullptr
               && dynamic_cast<const T*>(value.get()) != nullptr;
    }

    template <typename T>
    static T& get(OwningPointer<Base>& value)
    {
        return *dynamic_cast<T*>(value.get());
    }

    template <typename T>
    static T& assign(OwningPointer<Base>& value)
    {
        auto* derived = new T {};
        value.reset(derived);
        return *derived;
    }
};

} // namespace Miro::Detail

namespace Miro
{

template <typename Value>
class PolymorphicDispatcher
{
public:
    PolymorphicDispatcher(Reflector& refToUse, Value& valueToUse)
        : ref(refToUse)
        , value(valueToUse)
    {
        if (ref.isLoading())
        {
            auto keys = ref.mapKeys();
            if (!keys.empty())
                loadedTag = keys.front();
        }
    }

    template <typename T>
    void alt(std::string_view tag)
    {
        if (matched)
            return;

        auto childOpts = Detail::childOptionsFor<T>(ref.options());

        if (ref.isSaving())
        {
            if (Detail::PolymorphicAccess<Value>::template isActive<T>(value))
            {
                auto& active =
                    Detail::PolymorphicAccess<Value>::template get<T>(value);
                Detail::reflectValue(ref.atKey(tag, childOpts), active);
                matched = true;
            }
        }
        else
        {
            if (tag == loadedTag)
            {
                auto& slot =
                    Detail::PolymorphicAccess<Value>::template assign<T>(value);
                Detail::reflectValue(ref.atKey(tag, childOpts), slot);
                matched = true;
            }
        }
    }

    bool handled() const { return matched; }

private:
    Reflector& ref;
    Value& value;
    std::string loadedTag;
    bool matched = false;
};

template <typename Value, typename Callback>
void reflectPolymorphic(Reflector& ref, Value& value, Callback callback)
{
    ref.requirePolymorphicSupport("reflectPolymorphic");

    auto dispatcher = PolymorphicDispatcher<Value> {ref, value};
    callback(dispatcher);
}

template <typename Base, typename... Derived>
struct Polymorphic
{
    OwningPointer<Base> value;

    Base* get() const { return value.get(); }
    Base* operator->() const { return value.get(); }
    Base& operator*() const { return *value; }
    explicit operator bool() const { return value.get() != nullptr; }

    void reflect(Reflector& ref)
    {
        reflectPolymorphic(
            ref,
            value,
            [](auto& d)
            { (d.template alt<Derived>(Detail::typeNameOf<Derived>()), ...); });
    }
};

} // namespace Miro

namespace Miro::Detail
{

template <typename... Ts>
void reflectValue(Reflector& ref, std::variant<Ts...>& value)
{
    Miro::reflectPolymorphic(
        ref, value, [](auto& d) { (d.template alt<Ts>(typeNameOf<Ts>()), ...); });
}

} // namespace Miro::Detail
