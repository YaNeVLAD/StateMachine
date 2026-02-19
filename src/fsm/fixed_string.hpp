#ifndef FIXED_STRING_HPP
#define FIXED_STRING_HPP

#include <ostream>

namespace fsm
{
template <size_t N, typename T_Char>
struct base_fixed_string
{
	using traits_type = std::char_traits<T_Char>;
	using value_type = typename traits_type::char_type;

	T_Char data[N]{};

	constexpr base_fixed_string(const T_Char (&str)[N])
	{
		traits_type::copy(data, str, N);
	}

	[[nodiscard]] constexpr const T_Char* c_str() const
	{
		return data;
	}

	[[nodiscard]] constexpr std::string string() const
	{
		return data;
	}

	[[nodiscard]] constexpr size_t size() const
	{
		return N - 1;
	}

	template <size_t R>
	constexpr auto operator<=>(base_fixed_string<R, T_Char> const& rhs) const
	{
		return std::lexicographical_compare_three_way(
			data, data + N,
			rhs.data, rhs.data + R);
	}

	template <size_t R>
	constexpr bool operator==(const base_fixed_string<R, T_Char>& rhs) const
	{
		if (N != R)
		{
			return false;
		}

		return std::equal(data, data + N, rhs.data, rhs.data + R);
	}

	friend std::basic_ostream<T_Char>& operator<<(std::basic_ostream<T_Char>& os, const base_fixed_string& str)
	{
		return os << str.c_str();
	}
};

template <size_t N>
using fixed_string = base_fixed_string<N, char>;

template <size_t N>
using fixed_wstring = base_fixed_string<N, wchar_t>;
} // namespace fsm

#endif // FIXED_STRING_HPP
