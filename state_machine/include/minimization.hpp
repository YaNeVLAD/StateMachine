#ifndef MINIMIZATION_HPP
#define MINIMIZATION_HPP

#include "concepts.hpp"
#include "traits/minimization_traits.hpp"
#include "traits/state_machine_traits.hpp"

#include <map>
#include <set>
#include <vector>

namespace fsm
{
/**
 * @brief Minimizes a given deterministic finite state machine.
 *
 * This function creates a new, minimized state machine that is behaviorally
 * equivalent to the input machine. It implements a generic algorithm by
 * partitioning states into equivalence classes.
 *
 * For this function to work, the user must provide a full specialization of the
 * `minimization_traits` class for the `T_StateMachine` type. This traits class
 * must define how to access the machine's structure (e.g., get all states and
 * inputs) and how to reconstruct a new machine from the resulting partition.
 *
 * @tparam T_StateMachine The type of the state machine to be minimized. It must
 * satisfy the `concepts::state_machine` concept.
 * @param machine A constant reference to the state machine instance to minimize.
 * @return A new state machine instance with the minimum possible number of states.
 */
template <concepts::state_machine T_StateMachine>
T_StateMachine minimize(T_StateMachine const& machine)
{
	using machine_traits = state_machine_traits<T_StateMachine>;
	using min_traits = minimization_traits<T_StateMachine>;

	using state_type = typename machine_traits::state_type;
	using state_id = typename min_traits::id_type;
	using input_type = typename min_traits::input_type;
	using partition_t = std::vector<std::set<state_id>>;

	state_type const& current_state = machine.state();
	auto state_ids = min_traits::get_all_state_ids(current_state);
	auto inputs = min_traits::get_all_inputs(current_state);

	std::map<input_type, std::map<state_id, std::set<state_id>>> inverse_map;
	for (const auto& s_from : state_ids)
	{
		for (const auto& c : inputs)
		{
			auto s_next = min_traits::get_next_state_id(current_state, s_from, c);
			inverse_map[c][s_next].insert(s_from);
		}
	}

	partition_t partition;
	std::map<state_id, size_t> state_to_partition_index;

	for (const auto& id : state_ids)
	{
		bool placed = false;
		for (size_t i = 0; i < partition.size(); ++i)
		{
			if (min_traits::are_0_equivalent(current_state, id, *partition[i].begin()))
			{
				partition[i].insert(id);
				state_to_partition_index[id] = i;
				placed = true;
				break;
			}
		}
		if (!placed)
		{
			partition.push_back({ id });
			state_to_partition_index[id] = partition.size() - 1;
		}
	}

	std::set<size_t> worklist_set;

	size_t largest_set_size = 0;
	size_t largest_set_index = 0;
	for (size_t i = 0; i < partition.size(); ++i)
	{
		if (partition[i].size() > largest_set_size)
		{
			largest_set_size = partition[i].size();
			largest_set_index = i;
		}
	}
	for (size_t i = 0; i < partition.size(); ++i)
	{
		if (i != largest_set_index)
		{
			worklist_set.insert(i);
		}
	}

	while (!worklist_set.empty())
	{
		size_t splitter_index = *worklist_set.begin();
		worklist_set.erase(worklist_set.begin());

		auto splitter_A = partition[splitter_index];

		for (const auto& c : inputs)
		{
			std::set<state_id> pre_A_c;
			for (const auto& s_in_A : splitter_A)
			{
				if (inverse_map.count(c) && inverse_map.at(c).count(s_in_A))
				{
					const auto& sources = inverse_map.at(c).at(s_in_A);
					pre_A_c.insert(sources.begin(), sources.end());
				}
			}

			if (pre_A_c.empty())
				continue;

			std::vector<size_t> current_indices;
			for (size_t i = 0; i < partition.size(); ++i)
				current_indices.push_back(i);

			for (size_t i : current_indices)
			{
				if (i >= partition.size())
				{
					continue;
				}

				auto& R = partition[i];

				std::set<state_id> R1;
				std::set<state_id> R2;

				for (const auto& s_in_R : R)
				{
					if (pre_A_c.contains(s_in_R))
					{
						R1.insert(s_in_R);
					}
					else
					{
						R2.insert(s_in_R);
					}
				}

				if (!R1.empty() && !R2.empty())
				{
					partition[i] = R1;
					partition.push_back(R2);
					size_t R2_index = partition.size() - 1;

					for (const auto& s : R1)
					{
						state_to_partition_index[s] = i;
					}
					for (const auto& s : R2)
					{
						state_to_partition_index[s] = R2_index;
					}

					if (worklist_set.contains(i))
					{
						worklist_set.erase(i);
						worklist_set.insert(i);
						worklist_set.insert(R2_index);
					}
					else
					{
						if (R1.size() <= R2.size())
						{
							worklist_set.insert(i);
						}
						else
						{
							worklist_set.insert(R2_index);
						}
					}
				}
			}
		}
	}

	return min_traits::reconstruct_from_partition(machine, partition);
}
} // namespace fsm

#endif // MINIMIZATION_HPP
