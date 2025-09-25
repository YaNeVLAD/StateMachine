#ifndef MOORE_MACHINE_HPP
#define MOORE_MACHINE_HPP

#include "base_state_machine.hpp"

#include <utility>

class MooreMachine;

struct MooreState
{
	using State = std::string;
	using Input = std::string;
	using Output = std::string;
	using Transitions = std::map<std::pair<State, Input>, State>;

	std::set<State> states;
	std::map<State, Output> outputs;
	Transitions transitions;
	State start_state;
	State current_state;
};

template <>
struct fsm::state_machine_traits<MooreMachine>
{
	using state_type = MooreState;
	using input_type = MooreState::Input;
	using output_type = MooreState::Output;
};

class MooreMachine : public fsm::base_state_machine<MooreMachine>
{
	using Base = base_state_machine;
	using State = MooreState::State;
	using Input = MooreState::Input;
	using Output = MooreState::Output;

public:
	using TransitionResult = State;

	explicit MooreMachine(const MooreState& initialState)
		: Base(initialState)
	{
	}

	static TransitionResult translate(const Input& input, const MooreState& state)
	{
		const auto& transitions = state.transitions;
		const auto it = transitions.find({ state.current_state, input });

		if (it == transitions.end())
		{
			throw std::runtime_error("Undefined transition for state '" + state.current_state + "' with input '" + input + "'");
		}

		return it->second;
	}

	[[nodiscard]] Output output_from(TransitionResult const& result) const
	{
		const auto& outputs = state().outputs;
		const auto it = outputs.find(result);

		if (it == outputs.end())
		{
			throw std::runtime_error("Output for state '" + result + "' is not defined");
		}

		return it->second;
	}

	MooreState next_state_from(TransitionResult const& result)
	{
		current_state().current_state = result;
		return current_state();
	}
};

#endif // MOORE_MACHINE_HPP
