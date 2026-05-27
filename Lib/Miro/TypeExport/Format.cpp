#include "Format.h"

namespace Miro::TypeExport
{

namespace Detail
{

Vector<Format>& formatRegistry()
{
    static auto registry = Vector<Format> {};
    return registry;
}

} // namespace Detail

int registerFormat(Format format)
{
    Detail::formatRegistry().add(std::move(format));
    return 0;
}

} // namespace Miro::TypeExport
