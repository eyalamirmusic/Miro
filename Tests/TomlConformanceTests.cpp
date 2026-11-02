#include <Miro/Json.h>
#include <Miro/Toml.h>

#include <NanoTest/NanoTest.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace nano;
using namespace Miro;

// Runs toml-test's TOML 1.0 case list: every valid document must parse
// to the JSON its case expects and survive print() and a second parse;
// every invalid one must throw. Cases holding a date or time are
// counted as refused instead, since Miro has no type to load them into.

namespace TomlConformanceTests
{

constexpr auto testDirectory = std::string_view {MIRO_TOML_TEST_DIRECTORY};
constexpr auto caseListName = std::string_view {"files-toml-1.0.0"};
constexpr auto validPrefix = std::string_view {"valid/"};
constexpr auto tomlExtension = std::string_view {".toml"};

struct ExpectedValue
{
    Json::Value value;
    bool hasDateOrTime = false;
};

std::string readFile(const std::string& path)
{
    auto file = std::ifstream {path, std::ios::binary};
    auto contents = std::ostringstream {};
    contents << file.rdbuf();
    return contents.str();
}

std::vector<std::string> readCaseList()
{
    auto list = std::istringstream {
        readFile(std::string {testDirectory} + "/" + std::string {caseListName})};
    auto cases = std::vector<std::string> {};

    for (auto line = std::string {}; std::getline(list, line);)
    {
        // A Windows checkout gives the list, which is not marked -text, CRLF.
        if (line.ends_with('\r'))
            line.pop_back();

        if (line.ends_with(tomlExtension))
            cases.push_back(line);
    }

    return cases;
}

bool isTaggedScalar(const Json::Object& object)
{
    constexpr auto taggedScalarKeyCount = std::size_t {2};
    auto type = Json::find(object, "type");
    auto value = Json::find(object, "value");

    return object.size() == taggedScalarKeyCount && type != nullptr
           && value != nullptr && type->isString() && value->isString();
}

double parseTaggedFloat(const std::string& text)
{
    if (text == "nan" || text == "+nan" || text == "-nan")
        return std::nan("");

    if (text == "inf" || text == "+inf")
        return INFINITY;

    if (text == "-inf")
        return -INFINITY;

    return Json::parse(text).asNumber();
}

void decodeExpected(const Json::Value& tagged, ExpectedValue& result);

Json::Value decodeTaggedScalar(const Json::Object& object, ExpectedValue& result)
{
    auto& type = object.at("type").asString();
    auto& text = object.at("value").asString();

    if (type == "string")
        return Json::Value {text};
    if (type == "integer")
        return Json::Value {static_cast<std::int64_t>(std::stoll(text))};
    if (type == "float")
        return Json::Value {parseTaggedFloat(text)};
    if (type == "bool")
        return Json::Value {text == "true"};

    result.hasDateOrTime = true;
    return Json::Value {};
}

Json::Value decodeExpectedNode(const Json::Value& tagged, ExpectedValue& result)
{
    if (tagged.isArray())
    {
        auto elements = Json::Array {};

        for (auto& element: tagged.asArray().getVector())
            elements.add(decodeExpectedNode(element, result));

        return Json::Value {std::move(elements)};
    }

    auto& object = tagged.asObject();

    if (isTaggedScalar(object))
        return decodeTaggedScalar(object, result);

    auto decoded = Json::Object {};

    for (auto& [key, child]: object)
        decoded.emplace(key, decodeExpectedNode(child, result));

    return Json::Value {std::move(decoded)};
}

ExpectedValue decodeExpectedFile(const std::string& path)
{
    auto result = ExpectedValue {};
    result.value = decodeExpectedNode(Json::parse(readFile(path)), result);
    return result;
}

bool areEquivalent(const Json::Value& left, const Json::Value& right);

bool areEquivalentArrays(const Json::Array& left, const Json::Array& right)
{
    auto& leftElements = left.getVector();
    auto& rightElements = right.getVector();

    if (leftElements.size() != rightElements.size())
        return false;

    for (auto index = std::size_t {0}; index < leftElements.size(); ++index)
    {
        if (!areEquivalent(leftElements[index], rightElements[index]))
            return false;
    }

    return true;
}

bool areEquivalentObjects(const Json::Object& left, const Json::Object& right)
{
    if (left.size() != right.size())
        return false;

    for (auto& [key, value]: left)
    {
        auto other = Json::find(right, key);

        if (other == nullptr || !areEquivalent(value, *other))
            return false;
    }

    return true;
}

// Like Value::operator==, except that a NaN matches a NaN and an integer
// never matches a float, since TOML keeps the two apart.
bool areEquivalent(const Json::Value& left, const Json::Value& right)
{
    if (left.isInteger() != right.isInteger())
        return false;

    if (left.isNumber() && right.isNumber() && !left.isInteger())
    {
        auto bothNan = std::isnan(left.asNumber()) && std::isnan(right.asNumber());
        return bothNan || left.asNumber() == right.asNumber();
    }

    if (left.isArray() && right.isArray())
        return areEquivalentArrays(left.asArray(), right.asArray());

    if (left.isObject() && right.isObject())
        return areEquivalentObjects(left.asObject(), right.asObject());

    return left == right;
}

std::string pathWithoutExtension(const std::string& casePath)
{
    return casePath.substr(0, casePath.size() - tomlExtension.size());
}

std::string checkValidCase(const std::string& casePath)
{
    auto base = std::string {testDirectory} + "/" + pathWithoutExtension(casePath);
    auto expected = decodeExpectedFile(base + ".json");
    auto document = readFile(base + ".toml");

    try
    {
        auto parsed = Toml::parse(document);

        if (expected.hasDateOrTime)
            return "a document holding a date or time was not refused";

        if (!areEquivalent(parsed, expected.value))
            return "parsed to " + Json::print(parsed);

        auto printed = Toml::print(parsed);

        if (!areEquivalent(Toml::parse(printed), parsed))
            return "did not survive print():\n" + printed;
    }
    catch (const Toml::ParseError& error)
    {
        if (expected.hasDateOrTime)
            return {};

        return error.what();
    }

    return {};
}

std::string checkInvalidCase(const std::string& casePath)
{
    try
    {
        Toml::parse(readFile(std::string {testDirectory} + "/" + casePath));
    }
    catch (const Toml::ParseError&)
    {
        return {};
    }

    return "an invalid document parsed without an error";
}

std::string checkCase(const std::string& casePath)
{
    if (casePath.starts_with(validPrefix))
        return checkValidCase(casePath);

    return checkInvalidCase(casePath);
}

} // namespace TomlConformanceTests

using namespace TomlConformanceTests;

auto tomlConformance =
    test("TOML: passes the toml-test 1.0 valid and invalid cases") = []
{
    auto cases = readCaseList();
    auto failures = std::size_t {0};

    for (auto& casePath: cases)
    {
        auto failure = checkCase(casePath);

        if (failure.empty())
            continue;

        ++failures;
        std::cout << casePath << ": " << failure << "\n";
    }

    check(!cases.empty());
    check(failures == 0);
};
