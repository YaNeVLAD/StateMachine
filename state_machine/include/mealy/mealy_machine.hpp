#ifndef MEALY_MACHINE_HPP
#define MEALY_MACHINE_HPP

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include <base_state_machine.hpp>
#include <default_translator.hpp>

#include "mealy_state.hpp"
#include "mealy_state_machine_traits.hpp"
#include "mealy_translation_traits.hpp"

namespace fsm
{
class mealy_machine
	: public base_state_machine<mealy_machine>
	, public default_translator<mealy_machine>
{
	using base = base_state_machine;
	using output = mealy_state::output;
	using transition_result = mealy_state::transitions_t::mapped_type;

	friend class fsm::base_state_machine<mealy_machine>;
	friend class fsm::default_translator<mealy_machine>;

public:
	using state_type = mealy_state;

	explicit mealy_machine(mealy_state const& initial_state)
		: base(initial_state)
		, default_translator()
	{
	}

	explicit mealy_machine(mealy_state&& initial_state)
		: base(std::move(initial_state))
		, default_translator()
	{
	}

private:
	[[nodiscard]] static output output_from(transition_result const& result)
	{
		return result.second;
	}

	mealy_state next_state_from(transition_result const& result)
	{
		current_state().current_state_id = result.first;
		return current_state();
	}
};
} // namespace fsm

#endif // MEALY_MACHINE_HPP
