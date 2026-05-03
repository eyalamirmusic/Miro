// Registers the example types with the Miro type-export runner.
// Compiled directly into the MiroTypesExport executable (via SOURCES in
// miro_add_type_export) so these static initializers fire at startup.

#include "Types.h"

#include <Miro/TypeExport/Register.h>

MIRO_EXPORT_TYPES(User, Address, Role)
