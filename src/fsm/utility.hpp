#ifndef FSM_UTILITY_HPP
#define FSM_UTILITY_HPP

#include "traits/string_traits.hpp"

#include <algorithm>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <variant>

namespace fsm::utility
{
template <typename T>
struct trim_result
{
	using type = T;
};

template <typename T_Char, std::size_t N>
struct trim_result<T_Char[N]>
{
	using type = std::basic_string_view<T_Char>;
};

template <typename T_Char>
struct trim_result<const T_Char*>
{
	using type = std::basic_string_view<T_Char>;
};

template <typename T>
using trim_result_t = typename trim_result<std::remove_cvref_t<T>>::type;

template <typename T_String>
constexpr auto trim(
	const T_String& str,
	std::basic_string_view<typename string_traits_for<T_String>::char_type> delimiters = string_traits_for<T_String>::delimiters)
	-> trim_result_t<T_String>
{
	using char_type = typename string_traits_for<T_String>::char_type;
	using return_type = trim_result_t<T_String>;

	auto is_delimiter = [&delimiters](const char_type& c) {
		return delimiters.contains(c);
	};

	auto range = [&] {
		if constexpr (std::is_pointer_v<std::remove_cvref_t<T_String>>)
		{
			return std::basic_string_view<char_type>{ str };
		}
		else
		{
			return std::views::all(str);
		}
	}();

	auto start = std::ranges::find_if_not(range, is_delimiter);
	if (start == std::ranges::end(range))
	{
		return return_type{};
	}

	auto end = std::ranges::find_if_not(range | std::views::reverse, is_delimiter).base();

	return return_type{ start, end };
}

template <typename... Ts>
struct overloaded : Ts...
{
	using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename Variant, typename... Fs>
constexpr auto overloaded_visitor(Variant&& v, Fs&&... fs)
{
	return std::visit(overloaded{ std::forward<Fs>(fs)... }, std::forward<Variant>(v));
}

template <typename... Fs>
auto make_visitor(Fs&&... fs)
{
	return overloaded{ std::forward<Fs>(fs)... };
}
} // namespace fsm::utility

#endif // FSM_UTILITY_HPP
