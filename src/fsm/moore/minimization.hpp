#ifndef MOORE_MINIMIZATION_HPP
#define MOORE_MINIMIZATION_HPP

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../minimization.hpp"
#include "../moore_machine.hpp"
#include "../traits/minimization_traits.hpp"
#include "moore_state.hpp"

template <>
struct fsm::minimization_traits<fsm::moore_machine>
{
	using id_type = moore_machine::state_type::state_id;
	using input_type = moore_machine::state_type::input;

	static std::vector<id_type> get_all_state_ids(moore_state const& state)
	{
		auto const& s = state.state_ids;
		return { s.begin(), s.end() };
	}

	static std::vector<input_type> get_all_inputs(moore_state const& state)
	{
		std::set<input_type> inputs;
		for (auto const& [_, input] : state.transitions | std::views::keys)
		{
			inputs.insert(input);
		}
		return { inputs.begin(), inputs.end() };
	}

	static id_type get_next_state_id(
		moore_state const& state,
		id_type const& current,
		input_type const& input)
	{
		return state.transitions.at({ current, input });
	}

	static bool are_0_equivalent(
		moore_state const& state,
		id_type const& s1,
		id_type const& s2)
	{
		return state.outputs.at(s1) == state.outputs.at(s2);
	}

	static moore_machine reconstruct_from_partition(
		moore_machine const& original,
		std::vector<std::set<id_type>> const& partition)
	{
		moore_state minimalState;

		std::map<id_type, id_type> oldToNewIdMap;
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type newId = "s" + std::to_string(i);
			minimalState.state_ids.insert(newId);

			for (auto const& oldId : partition[i])
			{
				oldToNewIdMap[oldId] = newId;
			}
		}

		const id_type& originalStartId = original.state().initial_state_id;
		minimalState.initial_state_id = oldToNewIdMap.at(originalStartId);
		minimalState.current_state_id = minimalState.initial_state_id;

		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type newId = "s" + std::to_string(i);

			const id_type oldId = *partition[i].begin();
			auto const& output = original.state().outputs.at(oldId);

			minimalState.outputs[newId] = output;
		}

		const auto inputs = get_all_inputs(original.state());
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type newId = "s" + std::to_string(i);
			const id_type oldId = *partition[i].begin();

			for (auto const& input : inputs)
			{
				const id_type& originalId
					= original
						  .state()
						  .transitions
						  .at({ oldId, input });

				id_type const& newToId = oldToNewIdMap.at(originalId);

				minimalState.transitions[{ newId, input }] = newToId;
			}
		}

		return moore_machine(std::move(minimalState));
	}
};

#endif // MOORE_MINIMIZATION_HPP
