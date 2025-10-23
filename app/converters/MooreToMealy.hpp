#ifndef MOORE_TO_MEALY_HPP
#define MOORE_TO_MEALY_HPP

#include <../../state_machine/include/mealy_machine.hpp>
#include <../../state_machine/include/moore_machine.hpp>
#include <converter.hpp>
#include <stdexcept>

namespace details
{
inline mealy_state::transitions_t create_mealy_transitions(const moore_state& moore_state)
{
	mealy_state::transitions_t mealy_transitions;

	for (const auto& moore_transition : moore_state.transitions)
	{
		const auto& [from_state, input] = moore_transition.first;
		const auto& to_state = moore_transition.second;

		auto it = moore_state.outputs.find(to_state);
		if (it == moore_state.outputs.end())
		{
			throw std::runtime_error("Inconsistent Moore machine: No output defined for state '" + to_state + "'");
		}
		const auto& output_for_to_state = it->second;

		mealy_transitions[{ from_state, input }] = { to_state, output_for_to_state };
	}
	return mealy_transitions;
}
} // namespace details

using MooreToMealyConverter = fsm::converter<fsm::moore_machine, fsm::mealy_machine>;

template <>
struct fsm::converter<fsm::moore_machine, fsm::mealy_machine>
{
	[[nodiscard]] mealy_machine operator()(const moore_machine& moore) const
	{
		mealy_state mealyState;
		const auto& moore_state = moore.state();

		mealyState.state_ids = moore_state.state_ids;
		mealyState.initial_state_id = moore_state.initial_state_id;
		mealyState.current_state_id = moore_state.initial_state_id;

		mealyState.transitions = ::details::create_mealy_transitions(moore_state);

		return mealy_machine(mealyState);
	}
};

#endif // MOORE_TO_MEALY_HPP