#ifndef MINIMIZATION_HPP
#define MINIMIZATION_HPP

#include "concepts.hpp"
#include "traits/minimization_traits.hpp"
#include "traits/state_machine_traits.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace fsm::details
{
template <typename T_State>
using partition = std::vector<std::set<T_State>>;

template <typename T_State>
size_t find_partition_index(
	T_State const& state,
	std::vector<std::set<T_State>> const& partition)
{
	for (size_t i = 0; i < partition.size(); ++i)
	{
		if (partition[i].contains(state))
		{
			return i;
		}
	}
	return -1;
}
} // namespace fsm::details

namespace fsm
{
template <concepts::state_machine T_StateMachine>
T_StateMachine minimize(const T_StateMachine& fsm)
{
	using machine_traits = state_machine_traits<T_StateMachine>;
	using min_traits = minimization_traits<T_StateMachine>;
	using state_type = typename machine_traits::state_type;
	using state_id = typename min_traits::id_type;

	state_type const& current_state = fsm.state();

	auto state_ids = min_traits::get_all_state_ids(current_state);
	auto inputs = min_traits::get_all_inputs(current_state);

	details::partition<state_id> partition;
	for (auto const& id : state_ids)
	{
		bool placed = false;
		for (auto& part : partition)
		{
			if (min_traits::are_0_equivalent(current_state, id, *part.begin()))
			{
				part.insert(id);
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
		details::partition<state_id> new_partition;
		std::map<state_id, size_t> state_to_partition_map;

		for (size_t i = 0; i < partition.size(); ++i)
		{
			for (auto const& state : partition[i])
			{
				state_to_partition_map[state] = i;
			}
		}

		std::map<std::vector<size_t>, std::set<state_id>> refinement_groups;

		for (auto const& part : partition)
		{
			for (auto const& id : part)
			{
				std::vector<size_t> signature;
				for (auto const& input : inputs)
				{
					auto next_state = min_traits::get_next_state_id(current_state, id, input);
					signature.push_back(state_to_partition_map.at(next_state));
				}
				refinement_groups[signature].insert(id);
			}
		}

		for (auto const& pair : refinement_groups)
		{
			new_partition.push_back(pair.second);
		}

		if (new_partition.size() > partition.size())
		{
			partition = new_partition;
			changed = true;
		}

		if (!changed)
			break;
	}

	return min_traits::reconstruct_from_partition(fsm, partition);
}
} // namespace fsm

#endif // MINIMIZATION_HPP
