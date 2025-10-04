#ifndef MEALY_TO_MOORE_HPP
#define MEALY_TO_MOORE_HPP

#include "../MealyMachine.hpp"
#include "../MooreMachine.hpp"
#include "converter.hpp"

#include <ranges>
#include <set>
#include <string>
#include <utility>

namespace details
{
inline std::string ToMooreStateName(const std::string& mealy_state, const std::string& mealy_output)
{
	return mealy_state + " | " + mealy_output;
}

inline std::set<std::pair<std::string, std::string>>
CollectUniqueStateOutputPairs(const MealyState::Transitions& mealy_transitions)
{
	std::set<std::pair<std::string, std::string>> pairs;
	for (const auto& transition : mealy_transitions | std::views::values)
	{
		pairs.insert(transition);
	}

	return pairs;
}

inline void PopulateMooreStatesAndOutputs(
	const std::set<std::pair<std::string, std::string>>& pairs,
	MooreState& mooreState)
{
	for (const auto& [mealy_state_name, mealy_output] : pairs)
	{
		std::string new_moore_name = ToMooreStateName(mealy_state_name, mealy_output);
		mooreState.stateIds.insert(new_moore_name);
		mooreState.outputs[new_moore_name] = mealy_output;
	}
}

inline std::string CreateMooreStartState(const std::string& mealy_start_state, MooreState& mooreState)
{
	std::string moore_start_name = mealy_start_state + "_start";
	mooreState.stateIds.insert(moore_start_name);
	mooreState.outputs[moore_start_name] = "INITIAL";

	return moore_start_name;
}

inline MooreState::Transitions CreateMooreTransitions(
	const MealyState& mealy_state,
	const std::set<std::pair<std::string, std::string>>& pairs,
	const std::string& moore_start_name)
{
	MooreState::Transitions moore_transitions;

	for (const auto& [from, to] : mealy_state.transitions)
	{
		if (const auto& [from_state, input] = from; from_state == mealy_state.startStateId)
		{
			const auto& [to_state, output] = to;
			moore_transitions[{ moore_start_name, input }] = ToMooreStateName(to_state, output);
		}
	}

	for (const auto& [mealy_source_state, mealy_source_output] : pairs)
	{
		std::string moore_source_state = ToMooreStateName(mealy_source_state, mealy_source_output);
		for (const auto& [from, to] : mealy_state.transitions)
		{
			if (const auto& [from_state, input] = from; from_state == mealy_source_state)
			{
				const auto& [to_state, output] = to;
				moore_transitions[{ moore_source_state, input }] = ToMooreStateName(to_state, output);
			}
		}
	}

	return moore_transitions;
}
} // namespace details

using MealyToMooreConverter = fsm::converter<MealyMachine, MooreMachine>;

template <>
struct fsm::converter<MealyMachine, MooreMachine>
{
	[[nodiscard]] MooreMachine operator()(const MealyMachine& mealy) const
	{
		MooreState mooreState;
		const auto& mealy_state = mealy.state();

		const auto unique_pairs = ::details::CollectUniqueStateOutputPairs(mealy_state.transitions);

		::details::PopulateMooreStatesAndOutputs(unique_pairs, mooreState);

		mooreState.startStateId = mealy_state.startStateId;

		mooreState.transitions = ::details::CreateMooreTransitions(mealy_state, unique_pairs, mooreState.startStateId);

		mooreState.currentStateId = mooreState.startStateId;

		return MooreMachine(mooreState);
	}
};

#endif // MEALY_TO_MOORE_HPP