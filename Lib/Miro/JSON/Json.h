#pragma once

/** @file
    The raw JSON value type with its parser and printer (namespace
    Miro::Json), with no reflection.

    Value holds null, bool, a 64-bit integer, a double, a string, an Array
    or an Object (a std::map, so keys print sorted). parse() accepts line
    and block comments and throws ParseError; getParsedValue() returns
    null instead. The as*() accessors throw AccessError on a type
    mismatch. For struct to JSON serialization
    use toJSON and fromJSON from `<Miro/Reflect.h>`; this layer is what
    they produce and consume. Miro::JSON is an alias for Json::Value.
*/

#include "../Containers.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace Miro::Json
{

struct Value;

using Null = std::nullptr_t;
using Array = Vector<Value>;
using Object = std::map<std::string, Value, std::less<>>;

/// One JSON value of any kind. Integers and doubles are stored apart but
/// compare equal when numerically equal.
struct Value
{
    using Variant =
        std::variant<Null, bool, std::int64_t, double, std::string, Array, Object>;

    Value();
    Value(Null valueToUse);
    Value(bool valueToUse);
    Value(double valueToUse);
    Value(int valueToUse);
    Value(std::string valueToUse);
    Value(const char* valueToUse);
    Value(Array valueToUse);
    Value(Object valueToUse);

    // Copying is declared here and defaulted in Json.cpp rather than
    // left implicit. An implicit copy is inline, so every TU that copies
    // a JSON — which is every TU that includes the reflection layer,
    // through Json::Any — instantiates the std::variant copy and the
    // std::map<std::string, Value> tree copy behind it. Defining it once
    // in the library keeps that out of user TUs (~40ms per TU,
    // measured). Move and destruction stay implicit on purpose: they
    // are the parser's per-element hot path, and taking them out of
    // line costs ~12% on a large numeric array. Behaviour is identical
    // to the implicit versions.
    Value(const Value& otherToUse);
    Value& operator=(const Value& otherToUse);
    Value(Value&& otherToUse) noexcept = default;
    Value& operator=(Value&& otherToUse) noexcept = default;
    ~Value() = default;

    /// Every other integral width, stored exactly as an int64. An
    /// unsigned value past INT64_MAX has no exact slot, so it widens to
    /// double the way it did before there was an integer alternative.
    template <std::integral T>
        requires(!std::same_as<T, bool>)
    Value(T valueToUse)
    {
        constexpr auto mayExceedInt64 =
            std::unsigned_integral<T> && sizeof(T) >= sizeof(std::int64_t);

        if constexpr (mayExceedInt64)
        {
            constexpr auto largest =
                static_cast<T>(std::numeric_limits<std::int64_t>::max());

            if (valueToUse > largest)
            {
                data = static_cast<double>(valueToUse);
                return;
            }
        }

        data = static_cast<std::int64_t>(valueToUse);
    }

    template <std::floating_point T>
    Value(T valueToUse)
        : data(static_cast<double>(valueToUse))
    {
    }

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isInteger() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    /// Replaces any non-object value with an empty Object, then returns it.
    Object& toObject();

    /// The as*() accessors throw AccessError when the stored kind differs.
    /// asNumber() accepts an integer; asInteger() accepts a double only
    /// when it holds an exact integer.
    bool asBool() const;
    double asNumber() const;
    std::int64_t asInteger() const;
    const std::string& asString() const;
    const Array& asArray() const;
    const Object& asObject() const;
    Object& asObject();

    /// Conversions go through the as*() accessors and throw the same way.
    operator bool() const;
    operator int() const;
    operator std::int64_t() const;
    operator double() const;
    operator float() const;
    operator std::string() const;

    /// Unchecked against AccessError: a missing key throws std::out_of_range,
    /// a wrong kind std::bad_variant_access, and an index is not range checked.
    /// Use find() to look a key up without throwing.
    Value& operator[](const std::string& keyToUse);
    const Value& operator[](const std::string& keyToUse) const;
    Value& operator[](const char* keyToUse);
    const Value& operator[](const char* keyToUse) const;
    Value& operator[](std::size_t indexToUse);
    const Value& operator[](std::size_t indexToUse) const;

    Value& operator[](int indexToUse);
    const Value& operator[](int indexToUse) const;

    /// Numeric across the two storage kinds: 1 and 1.0 are the same
    /// number however each of them got here. Everything else is
    /// structural, so containers compare element by element.
    bool operator==(const Value& otherToUse) const;

    Variant data;
};

/// Base of everything the JSON layer throws, so one handler can catch a
/// malformed document and a bad access alike.
class Error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/// Thrown by parse() on malformed input or trailing content.
class ParseError : public Error
{
    using Error::Error;
};

/// Thrown by the as*() accessors and conversions on a type mismatch. The
/// message names the expected and actual type.
class AccessError : public Error
{
    using Error::Error;
};

/// Returns nullptr when the key is absent.
Value* find(Object& object, std::string_view key);
const Value* find(const Object& object, std::string_view key);

/// Helper used at the JSON-in seam of every command thunk. Lets handlers
/// accept `null` as a stand-in for "no fields" — fromJSON would otherwise
/// reject null when loading an object-shaped Req.
Value payloadOrEmpty(const Value& payload);

/// Throws ParseError on malformed input or trailing content.
Value parse(std::string_view inputToUse);

/// Like parse(), but never throws. On malformed input it catches the
/// ParseError (and any other exception) internally and returns a null
/// Value instead. Use this at boundaries where a default-constructed
/// result is preferable to handling a thrown ParseError.
Value getParsedValue(std::string_view inputToUse);

/// indentToUse is spaces per level; 0 prints on one line.
std::string print(const Value& valueToUse, int indentToUse = 0);
/// Prints to std::cout followed by a newline.
void log(const Value& valueToUse, int indentToUse = 0);
} // namespace Miro::Json

namespace Miro
{
using JSON = Json::Value;
} // namespace Miro
