#pragma once

namespace Miro
{
template <typename... Args>
constexpr void ignoreUnused(Args&&...) noexcept
{
}
} // namespace Miro
