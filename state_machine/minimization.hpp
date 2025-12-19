#ifndef MINIMIZATION_HPP
#define MINIMIZATION_HPP

#include <concepts.hpp>
#include <traits/minimization_traits.hpp>
#include <traits/state_machine_traits.hpp>

#include <map>
#include <set>
#include <stdexcept>
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
T_StateMachine minimize(const T_StateMachine& machine)
{
	using machine_traits = state_machine_traits<T_StateMachine>;
	using min_traits = minimization_traits<T_StateMachine>;

	using state_type = typename machine_traits::state_type;
	using state_id = typename min_traits::id_type;
	using partition_t = std::vector<std::set<state_id>>;

	state_type const& current_state = machine.state();
	auto state_ids = min_traits::get_all_state_ids(current_state);
	auto inputs = min_traits::get_all_inputs(current_state);

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
				placed = true;
				break;
			}
		}
		if (!placed)
		{
			partition.push_back({ id });
		}
	}

	while (true)
	{
		bool changed = false;
		partition_t new_partition;

		for (size_t i = 0; i < partition.size(); ++i)
		{
			for (const auto& state : partition[i])
			{
				state_to_partition_index[state] = i;
			}
		}

		const size_t SINK_PARTITION_INDEX = partition.size();

		for (const auto& part : partition)
		{
			std::map<std::vector<size_t>, std::set<state_id>> refinement_groups;

			for (const auto& id : part)
			{
				std::vector<size_t> signature;
				for (const auto& input : inputs)
				{
					try
					{
						auto next_state = min_traits::get_next_state_id(current_state, id, input);
						signature.push_back(state_to_partition_index.at(next_state));
					}
					catch (const std::out_of_range&)
					{
						signature.push_back(SINK_PARTITION_INDEX);
					}
				}
				refinement_groups[signature].insert(id);
			}

			for (auto const& [_, group] : refinement_groups)
			{
				new_partition.push_back(group);
			}
		}

		if (new_partition.size() > partition.size())
		{
			partition = new_partition;
			changed = true;
		}

		if (!changed)
			break;
	}

	return min_traits::reconstruct_from_partition(machine, partition);
}
} // namespace fsm

#endif // MINIMIZATION_HPP
