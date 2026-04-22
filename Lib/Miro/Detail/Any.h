#pragma once

#include "Serialize.h"

namespace Miro::Json
{

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
