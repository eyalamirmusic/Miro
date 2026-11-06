#pragma once

/** @file
    Entry header for the runtime command and event bridge: Bridge,
    ApiReflector, Event, CommandTable and Completer. Codegen-side pieces
    (DescribeReflector, the exporters) are in `<Miro/Codegen.h>`. */

#include "Bridge/ApiReflector.h"
#include "Bridge/BindReflector.h"
#include "Bridge/Bridge.h"
#include "Bridge/Callable.h"
#include "Bridge/Event.h"
#include "Reflection/CommandTable.h"
