#ifndef MEALY_TRANSLATION_TRAITS_HPP
#define MEALY_TRANSLATION_TRAITS_HPP

#include "mealy_state.hpp"
#include "traits/translation_traits.hpp"

namespace fsm
{
class mealy_machine;
}

template <>
struct fsm::translation_traits<fsm::mealy_machine>
{
	using find_result_type = mealy_state::transitions_t::const_iterator;
	using result_type = mealy_state::transitions_t::mapped_type;

	static find_result_type find(mealy_state const& state, mealy_state::input const& input)
	{
		return state.transitions.find({ state.current_state_id, input });
	}

	static bool is_valid(find_result_type const& find_result, mealy_state const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

#endif // MEALY_TRANSLATION_TRAITS_HPP
