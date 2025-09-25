#ifndef MEALY_MACHINE_HPP
#define MEALY_MACHINE_HPP

#include "base_state_machine.hpp"

#include <functional>
#include <set>
#include <utility>

struct MealyState
{
	using State = std::string;
	using Input = std::string;
	using Output = std::string;
	using Transitions = std::map<std::pair<State, Input>, std::pair<State, Output>>;

	Transitions transitions;
	std::set<std::string> states;
	std::string start_state;
	std::string current_state;
};

class MealyMachine;

template <>
struct fsm::state_machine_traits<MealyMachine>
{
	using state_type = MealyState;
	using input_type = MealyState::Input;
	using output_type = MealyState::Output;
};

class MealyMachine : public fsm::base_state_machine<MealyMachine>
{
	using Base = base_state_machine;
	using State = MealyState::State;
	using Input = MealyState::Input;
	using Output = MealyState::Output;

public:
	using TransitionResult = std::pair<State, Output>;

	explicit MealyMachine(MealyState const& initialState)
		: Base(initialState)
	{
	}

	[[nodiscard]] static TransitionResult translate(Input const& input, MealyState const& state)
	{
		auto const& transitions = state.transitions;
		const auto it = transitions.find({ state.current_state, input });

		if (it == transitions.end())
		{
			throw std::runtime_error("Undefined transition for state '" + state.current_state + "' with input '" + input + "'");
		}

		return it->second;
	}

	static Output output_from(TransitionResult const& result)
	{
		return result.second;
	}

	MealyState next_state_from(TransitionResult const& result)
	{
		current_state().current_state = result.first;
		return current_state();
	}
};

#endif // MEALY_MACHINE_HPP
