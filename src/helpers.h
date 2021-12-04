// Some convenience from C++23 that I want now
#pragma once
#include <cmath>
#include <utility>
#include <type_traits>
#include <ranges>

namespace blga {

inline constexpr auto enumerate = std::views::transform([start = 0](auto&& val) mutable {
    return std::make_pair(start++, std::forward<decltype(val)>(val));
});

template <class Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum>
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

// key == 0 is A2
inline auto note_frequency(int key) -> double
{
	constexpr double a2_frequency = 110.0;
	constexpr double twelfth_root_two = 1.05946309435929526456182529494634170077920; // BIG
	return a2_frequency * std::pow(twelfth_root_two, key);
}

}