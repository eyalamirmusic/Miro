#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Miro::Json
{

struct Value;

using Null = std::nullptr_t;
using Array = std::vector<Value>;
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

    bool asBool() const;
    double asNumber() const;
    const std::string& asString() const;
    const Array& asArray() const;
    const Object& asObject() const;

    explicit operator bool() const;
    explicit operator int() const;
    explicit operator double() const;
    explicit operator std::string() const;

    Value& operator[](const std::string& keyToUse);
    const Value& operator[](const std::string& keyToUse) const;
    Value& operator[](std::size_t indexToUse);
    const Value& operator[](std::size_t indexToUse) const;

    bool operator==(const Value& otherToUse) const = default;

    Variant data;
};

class ParseError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

inline Value* find(Object& object, std::string_view& key)
{
    auto it = object.find(std::string(key));

    if (it != object.end())
        return &it->second;

    return nullptr;
}

inline const Value* find(const Object& object, std::string_view key)
{
    auto it = object.find(std::string(key));

    if (it != object.end())
        return &it->second;

    return nullptr;
}

Value parse(std::string_view inputToUse);
std::string print(const Value& valueToUse);

} // namespace Miro::Json

namespace Miro
{
using JSON = Miro::Json::Value;
} // namespace Miro
