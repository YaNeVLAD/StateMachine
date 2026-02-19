#ifndef MEALY_MINIMIZATION_HPP
#define MEALY_MINIMIZATION_HPP

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../mealy_machine.hpp"
#include "../minimization.hpp"
#include "../traits/minimization_traits.hpp"
#include "mealy/mealy_state.hpp"

template <>
struct fsm::minimization_traits<fsm::mealy_machine>
{
	using id_type = mealy_machine::state_type::state_id;
	using input_type = mealy_machine::state_type::input;

	static std::vector<id_type> get_all_state_ids(mealy_state const& state)
	{
		auto const& s = state.state_ids;
		return { s.begin(), s.end() };
	}

	static std::vector<input_type> get_all_inputs(mealy_state const& state)
	{
		std::set<input_type> inputs;
		for (auto const& [_, input] : state.transitions | std::views::keys)
		{
			inputs.insert(input);
		}
		return { inputs.begin(), inputs.end() };
	}

	static id_type get_next_state_id(
		mealy_state const& state,
		id_type const& current,
		input_type const& input)
	{
		return state.transitions.at({ current, input }).first;
	}

	static bool are_0_equivalent(
		mealy_state const& state,
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

	static mealy_machine reconstruct_from_partition(
		mealy_machine const& original,
		std::vector<std::set<id_type>> const& partition)
	{
		mealy_state minimal;

		std::map<id_type, id_type> old_to_new_ids;
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type new_id = "s" + std::to_string(i);
			minimal.state_ids.insert(new_id);

			for (auto const& oldId : partition[i])
			{
				old_to_new_ids[oldId] = new_id;
			}
		}

		const id_type& original_start_id = original.state().initial_state_id;
		minimal.initial_state_id = old_to_new_ids.at(original_start_id);
		minimal.current_state_id = minimal.initial_state_id;

		const auto inputs = get_all_inputs(original.state());
		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type oldId = *partition[i].begin();
			const id_type newId = "s" + std::to_string(i);

			for (const auto& input : inputs)
			{
				const auto& [next_old_id, output]
					= original
						  .state()
						  .transitions
						  .at({ oldId, input });

				id_type const& new_to_id = old_to_new_ids.at(next_old_id);

				minimal.transitions[{ newId, input }] = { new_to_id, output };
			}
		}

		return mealy_machine(std::move(minimal));
	}
};

#endif // MEALY_MINIMIZATION_HPP
