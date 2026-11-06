#pragma once

/** @file Json::Any, a JSON value that converts from any reflectable type. */

#include "Serialize.h"

namespace Miro::Json
{

/// Converts implicitly from any reflectable type through toJSON, so one field can hold values of different types.
struct Any : Value
{
    using Value::Value;

    Any(const Value& valueToUse);
    Any(Value&& valueToUse);

    template <typename T>
    Any(T valueToUse)
        : Value(Miro::toJSON(valueToUse))
    {
    }
};

} // namespace Miro::Json
