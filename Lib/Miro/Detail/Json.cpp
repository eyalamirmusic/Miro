#include "Json.h"

namespace Miro::Json
{

Value::Value()
    : data(nullptr)
{
}

Value::Value(Null /*valueToUse*/)
    : data(nullptr)
{
}

Value::Value(bool valueToUse)
    : data(valueToUse)
{
}

Value::Value(double valueToUse)
    : data(valueToUse)
{
}

Value::Value(int valueToUse)
    : data(static_cast<double>(valueToUse))
{
}

Value::Value(std::string valueToUse)
    : data(std::move(valueToUse))
{
}

Value::Value(const char* valueToUse)
    : data(std::string(valueToUse))
{
}

Value::Value(Array valueToUse)
    : data(std::move(valueToUse))
{
}

Value::Value(Object valueToUse)
    : data(std::move(valueToUse))
{
}

bool Value::isNull() const
{
    return std::holds_alternative<Null>(data);
}

bool Value::isBool() const
{
    return std::holds_alternative<bool>(data);
}

bool Value::isNumber() const
{
    return std::holds_alternative<double>(data);
}

bool Value::isString() const
{
    return std::holds_alternative<std::string>(data);
}

bool Value::isArray() const
{
    return std::holds_alternative<Array>(data);
}

bool Value::isObject() const
{
    return std::holds_alternative<Object>(data);
}

Object& Value::toObject()
{
    if (!isObject())
        data = Object();

    return asObject();
}

bool Value::asBool() const
{
    return std::get<bool>(data);
}

double Value::asNumber() const
{
    return std::get<double>(data);
}

const std::string& Value::asString() const
{
    return std::get<std::string>(data);
}

const Array& Value::asArray() const
{
    return std::get<Array>(data);
}

const Object& Value::asObject() const
{
    return std::get<Object>(data);
}

Object& Value::asObject()
{
    return std::get<Object>(data);
}

Value::operator bool() const
{
    return asBool();
}
Value::operator int() const
{
    return static_cast<int>(asNumber());
}
Value::operator double() const
{
    return asNumber();
}
Value::operator float() const
{
    return static_cast<float>(asNumber());
}
Value::operator std::string() const
{
    return asString();
}

Value& Value::operator[](const std::string& keyToUse)
{
    return std::get<Object>(data).at(keyToUse);
}

const Value& Value::operator[](const std::string& keyToUse) const
{
    return std::get<Object>(data).at(keyToUse);
}

Value& Value::operator[](const char* keyToUse)
{
    return operator[](std::string(keyToUse));
}

const Value& Value::operator[](const char* keyToUse) const
{
    return operator[](std::string(keyToUse));
}

Value& Value::operator[](std::size_t indexToUse)
{
    return std::get<Array>(data).at(indexToUse);
}

const Value& Value::operator[](std::size_t indexToUse) const
{
    return std::get<Array>(data).at(indexToUse);
}
Value& Value::operator[](int indexToUse)
{
    return operator[]((size_t) indexToUse);
}

const Value& Value::operator[](int indexToUse) const
{
    return operator[]((size_t) indexToUse);
}

Value* find(Object& object, std::string_view key)
{
    auto it = object.find(std::string(key));

    if (it != object.end())
        return &it->second;

    return nullptr;
}

const Value* find(const Object& object, std::string_view key)
{
    auto it = object.find(std::string(key));

    if (it != object.end())
        return &it->second;

    return nullptr;
}

} // namespace Miro::Json
