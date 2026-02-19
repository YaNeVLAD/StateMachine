#ifndef MOORE_TRANSLATION_TRAITS_HPP
#define MOORE_TRANSLATION_TRAITS_HPP

#include "../traits/translation_traits.hpp"
#include "moore_state.hpp"

namespace fsm
{
class moore_machine;
}

template <>
struct fsm::translation_traits<fsm::moore_machine>
{
	using find_result_type = moore_state::transitions_t::const_iterator;
	using result_type = moore_state::transitions_t::mapped_type;

	static find_result_type find(moore_state const& state, moore_state::input const& input)
	{
		return state.transitions.find({ state.current_state_id, input });
	}

	static bool is_valid(find_result_type const& find_result, moore_state const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

#endif // MOORE_TRANSLATION_TRAITS_HPP
