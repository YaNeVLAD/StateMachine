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

template <>
struct fsm::translation_traits<MooreMachine>
{
	using find_result_type = MooreState::Transitions::const_iterator;
	using result_type = MooreState::Transitions::mapped_type;

	static find_result_type find(MooreState const& state, MooreState::Input const& input)
	{
		return state.transitions.find({ state.current_state, input });
	}

	static bool is_valid(find_result_type const& find_result, MooreState const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

class MooreMachine
	: public fsm::base_state_machine<MooreMachine>
	, public fsm::default_translator<MooreMachine>
{
	using Base = base_state_machine;
	using State = MooreState::State;
	using Input = MooreState::Input;
	using Output = MooreState::Output;

public:
	using TransitionResult = State;

	explicit MooreMachine(MooreState const& initialState)
		: Base(initialState)
	{
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
