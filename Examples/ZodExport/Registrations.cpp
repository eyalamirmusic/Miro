// Registers the example types with the Miro type-export runner.
// Compiled into the ZodExportRegistrations static library, which is then
// linked with WHOLE_ARCHIVE into the auto-exporter executable so these
// static initializers actually fire.

#include "Types.h"

#include <Miro/TypeExport/Register.h>

MIRO_EXPORT_TYPES(User, Address, Role)
