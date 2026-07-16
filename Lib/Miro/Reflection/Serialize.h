#pragma once

#include "JsonReflector.h"
#include "ReflectContainers.h"
#include "ReflectDispatch.h"
#include "TypeName.h"

#include <string>
#include <string_view>

// JSON serialization helpers only. The XML counterparts (toXML /
// fromXML / toXMLString / ...) live in SerializeXml.h so the JSON
// path — and everything built on it, like CommandTable — doesn't
// drag the XML layer into every consumer.

namespace Miro
{

template <typename T>
JSON toJSON(const T& value)
{
    auto json = JSON {};
    auto ref = JsonReflector {json, Detail::topLevelOptions<T>(Mode::Save)};
    Detail::reflectValue(ref, const_cast<T&>(value));
    return json;
}

// Loads `json` into `value`. Never throws: the built-in load path
// tolerates missing keys and mismatched types (they leave the
// corresponding field at its prior value), and any exception from a
// user-defined reflect() body — or from allocation — is caught here.
// On failure `value` keeps whatever was populated before the fault
// rather than propagating the exception.
template <typename T>
void fromJSON(T& value, const JSON& json)
{
    try
    {
        auto mutableJson = json;
        auto ref =
            JsonReflector {mutableJson, Detail::topLevelOptions<T>(Mode::Load)};
        Detail::reflectValue(ref, value);
    }
    catch (...)
    {
        // Intentionally swallowed — this function never throws.
    }
}

// Non-throwing (see fromJSON): a fault leaves the returned value with
// whatever was populated before it, starting from a default T {}.
template <typename T>
T createFromJSON(const JSON& json)
{
    auto value = T {};
    fromJSON(value, json);
    return value;
}

template <typename T>
std::string toJSONString(const T& value, int indent = 0)
{
    return Json::print(toJSON(value), indent);
}

template <typename T>
void logJSON(const T& value, int indent = 4)
{
    Json::log(toJSON(value), indent);
}

// Non-throwing: malformed JSON is swallowed by getParsedValue (yielding
// null, which loads as "no fields"), and fromJSON never throws.
template <typename T>
void fromJSONString(T& value, std::string_view jsonString)
{
    fromJSON(value, Json::getParsedValue(jsonString));
}

// Non-throwing counterpart of createFromJSON for a raw string; see
// fromJSONString.
template <typename T>
T createFromJSONString(std::string_view jsonString)
{
    return createFromJSON<T>(Json::getParsedValue(jsonString));
}

} // namespace Miro
