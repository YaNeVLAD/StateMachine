#ifndef FSM_INTEGER_SYMBOL_GENERATOR_H
#define FSM_INTEGER_SYMBOL_GENERATOR_H

#include "default_symbol_generator.hpp"

#include <concepts>
#include <set>

template <std::integral T_Int>
struct fsm::default_symbol_generator<T_Int, void>
{
	default_symbol_generator()
		: m_counter(0)
	{
		static_assert(sizeof(T_Int) <= sizeof(std::size_t),
			"Integer type is too large for this type");
	}

	T_Int next_start_symbol(const T_Int old_start, const std::set<T_Int>& nts)
	{
		auto max_found = static_cast<std::size_t>(old_start);

		if (!nts.empty())
		{
			max_found = std::max(max_found, static_cast<std::size_t>(*nts.rbegin()));
		}

		if (max_found >= std::numeric_limits<T_Int>::max())
		{
			throw std::overflow_error("integer_symbol_generator: no space left after the maximum symbol in the set.");
		}

		m_counter = max_found + 1;

		return static_cast<T_Int>(m_counter++);
	}

	T_Int next_terminal_proxy(T_Int)
	{
		return next_intermediate();
	}

	T_Int next_intermediate()
	{
		if (m_counter > std::numeric_limits<T_Int>::max())
		{
			throw std::overflow_error("integer_symbol_generator: T_Int range exhausted.");
		}
		return static_cast<T_Int>(m_counter++);
	}

private:
	std::size_t m_counter;
};

namespace fsm
{
template <std::integral T_Int>
using integer_symbol_generator = default_symbol_generator<T_Int>;
}

#endif // FSM_INTEGER_SYMBOL_GENERATOR_H
