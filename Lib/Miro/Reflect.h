#pragma once

/** @file
    Entry header for reflection and JSON serialization: MIRO_REFLECT,
    Reflector, toJSON, fromJSON, toJSONString and the rest. XML
    serialization is in `<Miro/Xml.h>`, the command and event bridge in
    `<Miro/Bridge.h>`. */

#include "Containers.h"
#include "IgnoreUnused.h"

#include "Reflection/Any.h"
#include "Reflection/JsonReflector.h"
#include "Reflection/Omittable.h"
#include "Reflection/ReflectContainers.h"
#include "Reflection/ReflectDispatch.h"
#include "Reflection/ReflectEnum.h"
#include "Reflection/ReflectJson.h"
#include "Reflection/ReflectMacro.h"
#include "Reflection/ReflectPolymorphic.h"
#include "Reflection/ReflectTagged.h"
#include "Reflection/Reflector.h"
#include "Reflection/Serialize.h"
#include "Reflection/TypeName.h"
