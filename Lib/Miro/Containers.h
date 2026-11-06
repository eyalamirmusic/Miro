#pragma once

/** @file
    Pulls in the ea_data_structures types Miro uses and names the common
    ones in namespace Miro (Vector, Array, OwningPointer, OwnedVector,
    Variant). Include this rather than the individual EA headers. */

#include <ea_data_structures/Pointers/OwningPointer.h>
#include <ea_data_structures/Structures/Array.h>
#include <ea_data_structures/Structures/MapVector.h>
#include <ea_data_structures/Structures/OwnedVector.h>
#include <ea_data_structures/Structures/Vector.h>
#include <ea_data_structures/ValueWrapper/Variant.h>

namespace Miro
{
using EA::Array;
using EA::OwnedVector;
using EA::OwningPointer;
using EA::Variant;
using EA::Vector;
} // namespace Miro
