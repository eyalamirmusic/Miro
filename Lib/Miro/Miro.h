#pragma once

/** @file
    Miro: C++20 reflection and what is built on it (namespace Miro). This
    umbrella header pulls in every public entry point and is kept for
    backwards compatibility; new code includes just the layer it needs:

    - `<Miro/Json.h>`: the raw JSON value type, parse and print
    - `<Miro/Reflect.h>`: reflection and JSON serialization (toJSON, fromJSON)
    - `<Miro/Xml.h>`: the XML value type and XML serialization
    - `<Miro/Bridge.h>`: the runtime command and event bridge
    - `<Miro/Codegen.h>`: the type-export and codegen toolchain
    - `<Miro/OpenAI.h>`: OpenAI function tools over a reflected API
    - `<Miro/Unicode.h>`: Unicode character properties and UTF-8
*/

#include "Bridge.h"
#include "Codegen.h"
#include "Json.h"
#include "OpenAI.h"
#include "Reflect.h"
#include "Unicode.h"
#include "Xml.h"
