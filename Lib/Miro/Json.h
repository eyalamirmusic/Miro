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

class Parser
{
public:
    static Value parse(std::string_view inputToUse);

private:
    explicit Parser(std::string_view inputToUse);

    Value parseValue();
    Value parseNull();
    Value parseBool();
    Value parseNumber();
    Value parseString();
    Value parseArray();
    Value parseObject();

    std::string parseStringRaw();
    char current() const;
    char advance();
    bool atEnd() const;
    void skipWhitespace();
    void expect(char charToUse);
    [[noreturn]] void error(const std::string& messageToUse) const;

    std::string_view input;
    std::size_t position;
};

} // namespace Miro::Json

namespace Miro
{
using JSON = Miro::Json::Value;
} // namespace Miro
