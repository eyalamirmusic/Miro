#include "Any.h"

#include <utility>

namespace Miro::Json
{

Any::Any(const Value& valueToUse)
    : Value(valueToUse)
{
}

Any::Any(Value&& valueToUse)
    : Value(std::move(valueToUse))
{
}

} // namespace Miro::Json
