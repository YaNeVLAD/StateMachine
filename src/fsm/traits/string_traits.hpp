#ifndef FSM_STRING_TRAITS_H
#define FSM_STRING_TRAITS_H

namespace fsm
{
template <typename T>
struct string_traits;

template <>
struct string_traits<char>
{
	static constexpr auto T_prefix = "T_";
	static constexpr auto C_prefix = "C_";
	static constexpr char quote = '\'';
};

template <>
struct string_traits<wchar_t>
{
	static constexpr auto T_prefix = L"T_";
	static constexpr auto C_prefix = L"C_";
	static constexpr wchar_t quote = L'\'';
};

template <>
struct string_traits<char8_t>
{
	static constexpr auto T_prefix = u8"T_";
	static constexpr auto C_prefix = u8"C_";
	static constexpr char8_t quote = u8'\'';
};

template <>
struct string_traits<char16_t>
{
	static constexpr auto T_prefix = u"T_";
	static constexpr auto C_prefix = u"C_";
	static constexpr char16_t quote = u'\'';
};

template <>
struct string_traits<char32_t>
{
	static constexpr auto T_prefix = U"T_";
	static constexpr auto C_prefix = U"C_";
	static constexpr char32_t quote = U'\'';
};
} // namespace fsm

#endif // FSM_STRING_TRAITS_H
