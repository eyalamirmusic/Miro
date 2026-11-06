#pragma once

/** @file Silences unused-variable warnings for any number of arguments. */

namespace Miro
{
/// Does nothing; marks its arguments as used.
template <typename... Args>
constexpr void ignoreUnused(Args&&...) noexcept
{
}
} // namespace Miro
