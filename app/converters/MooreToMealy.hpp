#ifndef MOORE_TO_MEALY_HPP
#define MOORE_TO_MEALY_HPP

#include "../MealyMachine.hpp"
#include "../MooreMachine.hpp"
#include "converter.hpp"
#include <stdexcept>

namespace details
{
inline MealyState::Transitions create_mealy_transitions(const MooreState& moore_state)
{
	MealyState::Transitions mealy_transitions;

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

using MooreToMealyConverter = fsm::converter<MooreMachine, MealyMachine>;

template <>
struct fsm::converter<MooreMachine, MealyMachine>
{
	[[nodiscard]] MealyMachine operator()(const MooreMachine& moore) const
	{
		MealyState mealyState;
		const auto& moore_state = moore.state();

		mealyState.stateIds = moore_state.stateIds;
		mealyState.startStateId = moore_state.startStateId;
		mealyState.currentStateId = moore_state.startStateId;

		mealyState.transitions = ::details::create_mealy_transitions(moore_state);

		return MealyMachine(mealyState);
	}
};

#endif // MOORE_TO_MEALY_HPP