#ifndef MOORE_MACHINE_HPP
#define MOORE_MACHINE_HPP

#include <map>
#include <ranges>
#include <string>
#include <utility>

#include <base_state_machine.hpp>
#include <default_translator.hpp>

#include "moore_state.hpp"
#include "moore_state_machine_traits.hpp"
#include "moore_translation_traits.hpp"

namespace fsm
{
class moore_machine
	: public base_state_machine<moore_machine>
	, public default_translator<moore_machine>
{
	using base = base_state_machine;
	using output = moore_state::output;
	using translation_result = moore_state::state_id;

	friend class fsm::base_state_machine<moore_machine>;
	friend class fsm::default_translator<moore_machine>;

public:
	using state_type = moore_state;

	explicit moore_machine(moore_state const& initialState)
		: base(initialState)
	{
	}

	explicit moore_machine(moore_state&& initialState)
		: base(std::move(initialState))
	{
	}

	[[nodiscard]] output output_from(translation_result const& result) const
	{
		const auto& outputs = state().outputs;
		const auto it = outputs.find(result);

		if (it == outputs.end())
		{
			throw std::runtime_error("Output for state '" + result + "' is not defined");
		}

		return it->second;
	}

	moore_state next_state_from(translation_result const& result)
	{
		current_state().current_state_id = result;
		return current_state();
	}
};
} // namespace fsm

#endif // MOORE_MACHINE_HPP
