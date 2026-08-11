#pragma once

#include "../Containers.h"

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
using Object = std::map<std::string, Value>;

struct Value
{
    using Variant = std::variant<Null, bool, double, std::string, Array, Object>;

    Value();
    Value(Null valueToUse);
    Value(bool valueToUse);
    Value(double valueToUse);
    Value(int valueToUse);
    Value(std::string valueToUse);
    Value(const char* valueToUse);
    Value(Array valueToUse);
    Value(Object valueToUse);

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    Object& toObject();

    bool asBool() const;
    double asNumber() const;
    const std::string& asString() const;
    const Array& asArray() const;
    const Object& asObject() const;
    Object& asObject();

    operator bool() const;
    operator int() const;
    operator double() const;
    operator float() const;
    operator std::string() const;

    Value& operator[](const std::string& keyToUse);
    const Value& operator[](const std::string& keyToUse) const;
    Value& operator[](const char* keyToUse);
    const Value& operator[](const char* keyToUse) const;
    Value& operator[](std::size_t indexToUse);
    const Value& operator[](std::size_t indexToUse) const;

    Value& operator[](int indexToUse);
    const Value& operator[](int indexToUse) const;

    bool operator==(const Value& otherToUse) const = default;

    Variant data;
};

class ParseError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

Value* find(Object& object, std::string_view key);
const Value* find(const Object& object, std::string_view key);

// Lets `null` stand in for "no fields": fromJSON rejects null when loading
// an object-shaped type.
Value payloadOrEmpty(const Value& payload);

Value parse(std::string_view inputToUse);

// Never throws: malformed input yields a null Value.
Value getParsedValue(std::string_view inputToUse);

std::string print(const Value& valueToUse, int indentToUse = 0);
void log(const Value& valueToUse, int indentToUse = 0);
} // namespace Miro::Json

namespace Miro
{
using JSON = Json::Value;
} // namespace Miro
