#ifndef MEALY_MACHINE_HPP
#define MEALY_MACHINE_HPP

#include "base_state_machine.hpp"
#include "default_translator.hpp"
#include "traits/translation_traits.hpp"

#include <map>
#include <set>
#include <string>
#include <utility>

struct MealyState
{
	using StateId = std::string;
	using Input = std::string;
	using Output = std::string;
	using Transitions = std::map<std::pair<StateId, Input>, std::pair<StateId, Output>>;

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

template <>
struct fsm::translation_traits<MealyMachine>
{
	using find_result_type = MealyState::Transitions::const_iterator;
	using result_type = MealyState::Transitions::mapped_type;

	static find_result_type find(MealyState const& state, MealyState::Input const& input)
	{
		return state.transitions.find({ state.current_state, input });
	}

	static bool is_valid(find_result_type const& find_result, MealyState const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

class MealyMachine
	: public fsm::base_state_machine<MealyMachine>
	, public fsm::default_translator<MealyMachine>
{
	using Base = base_state_machine;
	using StateId = MealyState::StateId;
	using Input = MealyState::Input;
	using Output = MealyState::Output;

public:
	using TransitionResult = std::pair<StateId, Output>;

	explicit MealyMachine(MealyState const& initialState)
		: Base(initialState)
		, default_translator()
	{
	}

	[[nodiscard]] static Output output_from(TransitionResult const& result)
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
