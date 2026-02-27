#ifndef FSM_STRING_TRAITS_H
#define FSM_STRING_TRAITS_H
#include "fsm/concepts.hpp"

namespace fsm
{
template <typename T>
struct string_traits;

template <>
struct string_traits<char>
{
	using char_type = char;

	static constexpr auto T_prefix = "T_";
	static constexpr auto C_prefix = "C_";
	static constexpr auto delimiters = " \t\r\n";
	static constexpr char quote = '\'';
};

template <>
struct string_traits<wchar_t>
{
	using char_type = wchar_t;

	static constexpr auto T_prefix = L"T_";
	static constexpr auto C_prefix = L"C_";
	static constexpr auto delimiters = L" \t\r\n";
	static constexpr wchar_t quote = L'\'';
};

template <>
struct string_traits<char8_t>
{
	using char_type = char8_t;

	static constexpr auto T_prefix = u8"T_";
	static constexpr auto C_prefix = u8"C_";
	static constexpr auto delimiters = u8" \t\r\n";
	static constexpr char8_t quote = u8'\'';
};

template <>
struct string_traits<char16_t>
{
	using char_type = char16_t;

	static constexpr auto T_prefix = u"T_";
	static constexpr auto C_prefix = u"C_";
	static constexpr auto delimiters = u" \t\r\n";
	static constexpr char16_t quote = u'\'';
};

template <>
struct string_traits<char32_t>
{
	using char_type = char32_t;

	static constexpr auto T_prefix = U"T_";
	static constexpr auto C_prefix = U"C_";
	static constexpr auto delimiters = U" \t\r\n";
	static constexpr char32_t quote = U'\'';
};

template <typename>
struct get_char_type
{
	using type = void;
};

template <concepts::has_value_type T>
struct get_char_type<T>
{
	using type = typename T::value_type;
};

template <typename T>
	requires std::is_pointer_v<std::decay_t<T>>
struct get_char_type<T>
{
	using type = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
};

template <typename T>
using string_traits_for = string_traits<typename get_char_type<T>::type>;

} // namespace fsm

#endif // FSM_STRING_TRAITS_H
