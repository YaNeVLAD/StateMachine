#ifndef MEALY_MACHINE_HPP
#define MEALY_MACHINE_HPP

#include "base_state_machine.hpp"
#include "default_translator.hpp"
#include "traits/minimization_traits.hpp"
#include "traits/translation_traits.hpp"

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

class MealyMachine;

struct MealyState
{
	using StateId = std::string;
	using Input = std::string;
	using Output = std::string;
	using Transitions = std::map<std::pair<StateId, Input>, std::pair<StateId, Output>>;

	Transitions transitions;
	std::set<std::string> stateIds;
	std::string startStateId;
	std::string currentStateId;
};

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
		return state.transitions.find({ state.currentStateId, input });
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

	explicit MealyMachine(MealyState&& initialState)
		: Base(std::move(initialState))
		, default_translator()
	{
	}

	[[nodiscard]] static Output output_from(TransitionResult const& result)
	{
		return result.second;
	}

	MealyState next_state_from(TransitionResult const& result)
	{
		current_state().currentStateId = result.first;
		return current_state();
	}
};

template <>
struct fsm::minimization_traits<MealyMachine>
{
	using id_type = std::string;
	using input_type = base_state_machine<MealyMachine>::input_type;

	static std::vector<id_type> get_all_state_ids(MealyState const& state)
	{
		auto const& s = state.stateIds;
		return { s.begin(), s.end() };
	}

	static std::vector<input_type> get_all_inputs(MealyState const& state)
	{
		std::set<input_type> inputs;
		for (auto const& [_, input] : state.transitions | std::views::keys)
		{
			inputs.insert(input);
		}
		return { inputs.begin(), inputs.end() };
	}

	static id_type get_next_state_id(
		MealyState const& state,
		id_type const& current,
		input_type const& input)
	{
		return state.transitions.at({ current, input }).first;
	}

	static bool are_0_equivalent(
		MealyState const& state,
		id_type const& s1,
		id_type const& s2)
	{
		return std::ranges::all_of(get_all_inputs(state),
			[state, s1, s2](std::string const& input) {
				const auto out1 = state.transitions.at({ s1, input }).second;
				const auto out2 = state.transitions.at({ s2, input }).second;

				return out1 == out2;
			});
	}

	static MealyMachine reconstruct_from_partition(
		MealyMachine const& original,
		std::vector<std::set<id_type>> const& partition)
	{
		MealyState minimalState;

		std::map<id_type, id_type> oldToNewIds;
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type new_id = "s" + std::to_string(i);
			minimalState.stateIds.insert(new_id);

			for (auto const& oldId : partition[i])
			{
				oldToNewIds[oldId] = new_id;
			}
		}

		const id_type& originalStartId = original.state().startStateId;
		minimalState.startStateId = oldToNewIds.at(originalStartId);
		minimalState.currentStateId = minimalState.startStateId;

		const auto inputs = get_all_inputs(original.state());
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type oldId = *partition[i].begin();
			const id_type newId = "s" + std::to_string(i);

			for (const auto& input : inputs)
			{
				const auto& [nextOldId, output]
					= original
						  .state()
						  .transitions
						  .at({ oldId, input });

				const id_type new_to_id = oldToNewIds.at(nextOldId);

				minimalState.transitions[{ newId, input }] = { new_to_id, output };
			}
		}

		return MealyMachine(std::move(minimalState));
	}
};

#endif // MEALY_MACHINE_HPP
