#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

#include "base_state_machine.hpp"
#include "converter.hpp"
#include "default_translator.hpp"
#include "traits/state_machine_traits.hpp"
#include "traits/translation_traits.hpp"

#include <map>
#include <set>
#include <string>

#include "mealy_machine.hpp"
#include "moore_machine.hpp"

namespace fsm
{
class recognizer;

struct recognizer_state
{
	using state_id = std::string;
	using input = std::string;
	using transitions_t = std::map<std::pair<state_id, input>, state_id>;

	std::set<state_id> state_ids;
	transitions_t transitions;
	state_id initial_state_id;
	state_id current_state_id;

	std::set<state_id> final_state_ids;
};

template <>
struct state_machine_traits<recognizer>
{
	using state_type = recognizer_state;
	using input_type = recognizer_state::input;
	using output_type = bool;
};

template <>
struct translation_traits<recognizer>
{
	using find_result_type = recognizer_state::transitions_t::const_iterator;
	using result_type = recognizer_state::transitions_t::mapped_type;

	static find_result_type find(recognizer_state const& state, recognizer_state::input const& input)
	{
		return state.transitions.find({ state.current_state_id, input });
	}

	static bool is_valid(find_result_type const& find_result, recognizer_state const& state)
	{
		return find_result != state.transitions.end();
	}

	static result_type result(find_result_type const& find_result)
	{
		return find_result->second;
	}
};

template <>
struct converter<moore_machine::state_type, recognizer_state>
{
	static recognizer_state operator()(moore_machine::state_type const& moore_state,
		std::set<recognizer_state::state_id> const& final_state_ids)
	{
		return recognizer_state{
			.state_ids = moore_state.state_ids,
			.transitions = moore_state.transitions,
			.initial_state_id = moore_state.initial_state_id,
			.current_state_id = moore_state.current_state_id,
			.final_state_ids = final_state_ids
		};
	}
};

template <>
struct converter<mealy_machine::state_type, recognizer_state>
{
	static recognizer_state operator()(mealy_machine::state_type const& mealy_state,
		std::set<recognizer_state::state_id> const& final_state_ids)
	{
		recognizer_state::transitions_t transitions;
		for (auto const& [key, val] : mealy_state.transitions)
		{
			transitions[key] = val.first;
		}

		return recognizer_state{
			.state_ids = mealy_state.state_ids,
			.transitions = transitions,
			.initial_state_id = mealy_state.initial_state_id,
			.current_state_id = mealy_state.current_state_id,
			.final_state_ids = final_state_ids
		};
	}
};

class recognizer
	: public base_state_machine<recognizer>
	, public default_translator<recognizer>
{
	using base = base_state_machine;
	using state_id = recognizer_state::state_id;
	using input_type = state_machine_traits<recognizer>::input_type;
	using output_type = state_machine_traits<recognizer>::output_type;
	using translation_result = state_id;

	friend class fsm::base_state_machine<recognizer>;
	friend class fsm::default_translator<recognizer>;

public:
	using state_type = recognizer_state;

	explicit recognizer(recognizer_state const& initial_state)
		: base(initial_state)
		, default_translator()
	{
	}

	explicit recognizer(recognizer_state&& initial_state)
		: base(std::move(initial_state))
		, default_translator()
	{
	}

	explicit recognizer(mealy_machine const& mealy, std::set<state_id> const& final_state_ids)
		: base(fsm::converter<mealy_machine::state_type, state_type>{}(mealy.state(), final_state_ids))
		, default_translator()
	{
	}

	explicit recognizer(moore_machine const& moore, std::set<state_id> const& final_state_ids)
		: base(fsm::converter<moore_machine::state_type, state_type>{}(moore.state(), final_state_ids))
		, default_translator()
	{
	}

	output_type handle_input(std::vector<input_type> const& inputs)
	{
		bool result = false;
		for (auto const& input : inputs)
		{
			result = base::handle_input(input);
		}
		return result;
	}

	template <typename... Args>
	output_type handle_input(Args&&... inputs)
	{
		(base::handle_input(inputs), ...);

		return is_final(state().current_state_id);
	}

	output_type handle_input(input_type const& input)
	{
		base::handle_input(input);
		return is_final(state().current_state_id);
	}

private:
	[[nodiscard]] output_type output_from(translation_result const& translation_result) const
	{
		return is_final(translation_result);
	}

	state_type next_state_from(translation_result const& translation_result)
	{
		current_state().current_state_id = translation_result;
		return current_state();
	}

	[[nodiscard]] output_type is_final(state_id const& state_id) const
	{
		return state().final_state_ids.contains(state_id);
	}
};

template <>
struct fsm::minimization_traits<recognizer>
{
	using state_type = recognizer_state;
	using id_type = state_type::state_id;
	using input_type = state_type::input;

	static std::vector<id_type> get_all_state_ids(state_type const& state)
	{
		auto const& s = state.state_ids;
		return { s.begin(), s.end() };
	}

	static std::vector<input_type> get_all_inputs(state_type const& state)
	{
		std::set<input_type> inputs;
		for (auto const& [_, input] : state.transitions | std::views::keys)
		{
			inputs.insert(input);
		}
		return { inputs.begin(), inputs.end() };
	}

	static id_type get_next_state_id(
		state_type const& state,
		id_type const& current,
		input_type const& input)
	{
		return state.transitions.at({ current, input });
	}

	static bool are_0_equivalent(
		state_type const& state,
		id_type const& s1,
		id_type const& s2)
	{
		const bool s1_is_final = state.final_state_ids.contains(s1);
		const bool s2_is_final = state.final_state_ids.contains(s2);
		return s1_is_final == s2_is_final;
	}

	static recognizer reconstruct_from_partition(
		recognizer const& original,
		std::vector<std::set<id_type>> const& partition)
	{
		state_type minimalState;
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

		const auto inputs = get_all_inputs(original.state());

		for (size_t i = 0; i < partition.size(); ++i)
		{
			const id_type newId = "s" + std::to_string(i);
			const id_type oldId = *partition[i].begin(); // Представитель класса

			if (original.state().final_state_ids.contains(oldId))
			{
				minimalState.final_state_ids.insert(newId);
			}

			for (auto const& input : inputs)
			{
				const id_type& originalNextId = original.state().transitions.at({ oldId, input });
				minimalState.transitions[{ newId, input }] = oldToNewIdMap.at(originalNextId);
			}
		}

		return recognizer(std::move(minimalState));
	}
};
} // namespace fsm

#endif // RECOGNIZER_HPP
