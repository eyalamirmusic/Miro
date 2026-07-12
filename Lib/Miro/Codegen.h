#pragma once

// Entry header: the codegen / type-export toolchain —
// DescribeReflector, TypeTree, Schema, the TypeScript / C++ emitters,
// command export, and the codegenMain() runner. Only codegen
// executables need this; app runtime code wants <Miro/Bridge.h> or
// <Miro/Reflect.h> instead.

#include "Bridge/DescribeReflector.h"
#include "CommandExport/CommandEntry.h"
#include "CommandExport/CommandExport.h"
#include "CommandExport/ResolvedTypes.h"
#include "Cpp/Cpp.h"
#include "Cpp/CppClient.h"
#include "Schema/Schema.h"
#include "TypeExport/CodegenMain.h"
#include "TypeExport/Context.h"
#include "TypeExport/Format.h"
#include "TypeScript/TypeScript.h"
#include "TypeTree/TypeTree.h"
