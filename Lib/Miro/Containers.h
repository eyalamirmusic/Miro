#pragma once

// Code needing an EA container includes this instead of the individual EA
// headers, so user TUs never depend on the ea_data_structures include layout.

#include <ea_data_structures/Pointers/OwningPointer.h>
#include <ea_data_structures/Structures/Array.h>
#include <ea_data_structures/Structures/MapVector.h>
#include <ea_data_structures/Structures/OwnedVector.h>
#include <ea_data_structures/Structures/Vector.h>

namespace Miro
{
using EA::Array;
using EA::OwnedVector;
using EA::OwningPointer;
using EA::Vector;
} // namespace Miro
