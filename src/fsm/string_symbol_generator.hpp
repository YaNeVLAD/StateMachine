#ifndef FSM_STRING_SYMBOL_GENERATOR_H
#define FSM_STRING_SYMBOL_GENERATOR_H

#include "concepts.hpp"
#include "default_symbol_generator.hpp"
#include "traits/string_traits.hpp"

#include <array>
#include <charconv>
#include <set>

template <fsm::concepts::is_string_like T_String>
struct fsm::default_symbol_generator<T_String, void>
{
	using char_type = typename T_String::value_type;
	using traits = string_traits<char_type>;

	explicit default_symbol_generator(
		char_type quote = traits::quote,
		const char_type* terminal_prefix = traits::T_prefix,
		const char_type* intermediate_prefix = traits::C_prefix)
		: m_quote{ quote }
		, m_t_prefix{ terminal_prefix }
		, m_c_prefix{ intermediate_prefix }
		, m_counter(1)
	{
	}

	T_String next_start_symbol(const T_String& old_start, const std::set<T_String>& nts)
	{
		T_String new_start = old_start + m_quote;
		while (nts.contains(new_start))
		{
			new_start += m_quote;
		}

		return new_start;
	}

	T_String next_terminal_proxy(const T_String& terminal)
	{
		return T_String(m_t_prefix) + terminal;
	}

	T_String next_intermediate()
	{
		std::array<char, 21> buffer{};
		auto [ptr, _] = std::to_chars(
			buffer.data(),
			buffer.data() + buffer.size(),
			m_counter++);

		T_String result(m_c_prefix);
		for (const char* p = buffer.data(); p != ptr; ++p)
		{
			result += static_cast<char_type>(*p);
		}
		return result;
	}

private:
	char_type m_quote;
	const char_type* m_t_prefix;
	const char_type* m_c_prefix;

	std::size_t m_counter;
};

namespace fsm
{
template <concepts::is_string_like T_String>
using string_symbol_generator = default_symbol_generator<T_String>;
}

#endif // FSM_STRING_SYMBOL_GENERATOR_H
