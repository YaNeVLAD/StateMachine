#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

#include "base_state_machine.hpp"
#include "converter.hpp"
#include "default_translator.hpp"
#include "traits/minimization_traits.hpp"
#include "traits/state_machine_traits.hpp"
#include "traits/translation_traits.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "mealy_machine.hpp"
#include "moore_machine.hpp"
#include "recognizer.hpp"

#include <fstream>
#include <queue>
#include <regex>
#include <sstream>

namespace fsm
{
class recognizer;

struct recognizer_state
{
	using state_id = std::string;
	using input = std::optional<std::string>;
	using transitions_t = std::multimap<std::pair<state_id, input>, state_id>;

	std::set<state_id> state_ids{};
	transitions_t transitions{};
	state_id initial_state_id{};
	state_id current_state_id{};

	std::set<state_id> final_state_ids{};

	bool is_deterministic{};
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
			.transitions = { moore_state.transitions.begin(), moore_state.transitions.end() },
			.initial_state_id = moore_state.initial_state_id,
			.current_state_id = moore_state.current_state_id,
			.final_state_ids = final_state_ids,
			.is_deterministic = true
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
			transitions.emplace(key, val.first);
		}

		return recognizer_state{
			.state_ids = mealy_state.state_ids,
			.transitions = transitions,
			.initial_state_id = mealy_state.initial_state_id,
			.current_state_id = mealy_state.current_state_id,
			.final_state_ids = final_state_ids,
			.is_deterministic = true
		};
	}
};

namespace details
{
inline std::string unquote(std::string str)
{
	if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
	{
		return str.substr(1, str.length() - 2);
	}
	return str;
}

inline std::string quote(const std::string& s)
{
	return "\"" + s + "\"";
}

inline void export_recognizer_to_dot(const recognizer_state& state, const std::string& filename)
{
	std::ofstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file for writing: " + filename);
	}

	file << "digraph Recognizer {\n";
	file << "    rankdir=LR;\n\n";

	file << "    // Start state pointer\n";
	file << "    " << quote(state.initial_state_id) << ";\n\n";

	file << "    // States\n";
	for (const auto& id : state.state_ids)
	{
		file << "    " << quote(id) << " [";

		file << "label = " << quote(id);

		if (state.final_state_ids.contains(id))
		{
			file << ", final = true, shape = doublecircle";
		}
		else
		{
			file << ", final = false, shape = circle";
		}

		file << "];\n";
	}
	file << "\n";

	file << "    // Transitions\n";
	for (const auto& [state_and_input, next_state] : state.transitions)
	{
		const auto& [from_id, input_opt] = state_and_input;
		const auto& to_id = next_state;

		file << "    " << quote(from_id)
			 << " -> " << quote(to_id);

		if (input_opt.has_value())
		{
			file << " [label = " << quote(input_opt.value()) << "]";
		}

		file << ";\n";
	}

	file << "}\n";
	file.close();
}

inline recognizer_state create_recognizer_from_dot(const std::string& filename)
{
	using state_id = recognizer_state::state_id;
	using input = recognizer_state::input;

	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filename);
	}

	recognizer_state state;
	bool is_deterministic = true;

	std::map<std::pair<state_id, input>, int> transition_counts;

	const std::string id_regex = R"lit((\w+|"[^"]+"))lit";

	std::regex edge_regex(
		R"lit(^\s*)lit" + id_regex + R"lit(\s*->\s*)lit" + id_regex + R"lit(\s*(?:\[\s*label\s*=\s*"([^"]*)"\s*\])?\s*;*$)lit");

	std::regex node_regex(
		R"lit(^\s*)lit" + id_regex + R"lit(\s*(?:\[([^\]]*)\])?\s*;*$)lit");

	std::regex final_attr_regex(R"lit(final\s*=\s*true)lit");

	std::string line;
	std::smatch matches;

	while (std::getline(file, line))
	{
		if (std::regex_match(line, matches, edge_regex))
		{
			state_id from = details::unquote(matches[1]);
			state_id to = details::unquote(matches[2]);
			input trans_input;

			if (matches[3].matched)
			{
				trans_input = matches[3].str();
			}
			else
			{
				is_deterministic = false;
			}

			state.state_ids.insert(from);
			state.state_ids.insert(to);
			state.transitions.emplace(std::make_pair(from, trans_input), to);

			transition_counts[{ from, trans_input }]++;
			if (transition_counts[{ from, trans_input }] > 1)
			{
				is_deterministic = false;
			}
		}
		else if (std::regex_match(line, matches, node_regex))
		{
			state_id id = fsm::details::unquote(matches[1]);
			state.state_ids.insert(id);

			if (state.initial_state_id.empty())
			{
				state.initial_state_id = id;
			}

			if (matches[2].matched)
			{
				if (std::string attrs = matches[2]; std::regex_search(attrs, final_attr_regex))
				{
					state.final_state_ids.insert(id);
				}
			}
		}
	}

	if (state.initial_state_id.empty())
	{
		throw std::runtime_error("No states defined in DOT file.");
	}

	state.current_state_id = state.initial_state_id;
	state.is_deterministic = is_deterministic;

	return state;
}
} // namespace details

class recognizer final
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

	template <concepts::container T_Container>
	output_type handle_input(T_Container&& inputs)
	{
		bool result = false;
		for (auto const& input : inputs)
		{
			result = base::handle_input(input);
		}

		return result;
	}

	template <std::convertible_to<input_type>... Args>
	output_type handle_input(Args&&... inputs)
	{
		(base::handle_input(inputs), ...);

		return is_final(state().current_state_id);
	}

	bool is_deterministic()
	{
		return current_state().is_deterministic;
	}

	static recognizer from_dot(std::string const& filename)
	{
		return recognizer{ std::move(details::create_recognizer_from_dot(filename)) };
	}

	void to_dot(std::string const& filename)
	{
		details::export_recognizer_to_dot(current_state(), filename);
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
struct minimization_traits<recognizer>
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
		return state.transitions.find({ current, input })->second;
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
			const id_type oldId = *partition[i].begin();

			if (original.state().final_state_ids.contains(oldId))
			{
				minimalState.final_state_ids.insert(newId);
			}

			for (auto const& input : inputs)
			{
				const id_type& originalNextId = original.state().transitions.find({ oldId, input })->second;
				std::pair p = { newId, input };
				std::pair p2 = { p, oldToNewIdMap.at(originalNextId) };
				minimalState.transitions.emplace(p2);
			}
		}

		return recognizer(std::move(minimalState));
	}
};

namespace details
{
inline void epsilon_closure_recursive(
	recognizer_state::state_id const& state_id,
	std::set<recognizer_state::state_id>& visited,
	recognizer_state::transitions_t const& nfa_trans)
{
	visited.insert(state_id);
	auto [it, end_it] = nfa_trans.equal_range({ state_id, std::nullopt });
	for (; it != end_it; ++it)
	{
		if (!visited.contains(it->second))
		{
			epsilon_closure_recursive(it->second, visited, nfa_trans);
		}
	}
}

inline std::set<recognizer_state::state_id>
epsilon_closure(
	std::set<recognizer_state::state_id> const& states,
	recognizer_state::transitions_t const& nfa_trans)
{
	std::set<recognizer_state::state_id> result;
	for (auto const& s : states)
	{
		epsilon_closure_recursive(s, result, nfa_trans);
	}
	return result;
}

inline std::set<recognizer_state::state_id>
move(std::set<recognizer_state::state_id> const& states,
	recognizer_state::input const& c,
	recognizer_state::transitions_t const& nfa_trans)
{
	std::set<recognizer_state::state_id> result;
	for (auto const& s : states)
	{
		auto [it, end_it] = nfa_trans.equal_range({ s, c });
		for (; it != end_it; ++it)
		{
			result.insert(it->second);
		}
	}
	return result;
}

inline std::string name_from_set(
	std::set<recognizer_state::state_id> const& set)
{
	std::stringstream ss;
	ss << "s_";
	for (auto const& id : set)
	{
		ss << id;
	}
	return ss.str();
}

inline std::set<recognizer_state::input>
get_nfa_alphabet(recognizer_state::transitions_t const& nfa_trans)
{
	std::set<recognizer_state::input> alphabet;
	for (const auto& [_, input] : nfa_trans | std::views::keys)
	{
		if (input.has_value())
		{
			alphabet.insert(input.value());
		}
	}
	return alphabet;
}
} // namespace details

inline recognizer determinize(recognizer const& recognizer)
{
	using namespace details;

	using state_id = recognizer_state::state_id;
	using state_set = std::set<state_id>;

	if (recognizer.state().is_deterministic)
	{
		return recognizer;
	}

	auto& nfa_state = recognizer.state();
	auto const& nfa_trans = nfa_state.transitions;
	const auto alphabet = get_nfa_alphabet(nfa_trans);

	recognizer_state result;

	std::map<state_set, state_id> dfa_state_names;
	std::queue<state_set> worklist;

	state_set start_set = epsilon_closure({ nfa_state.initial_state_id }, nfa_trans);
	result.initial_state_id = name_from_set(start_set);
	result.state_ids.insert(result.initial_state_id);
	dfa_state_names[start_set] = result.initial_state_id;
	worklist.push(start_set);

	while (!worklist.empty())
	{
		state_set current_set = worklist.front();
		worklist.pop();
		std::string current_dfa_name = dfa_state_names.at(current_set);

		for (auto const& s : current_set)
		{
			if (nfa_state.final_state_ids.contains(s))
			{
				result.final_state_ids.insert(current_dfa_name);
				break;
			}
		}

		for (auto const& c : alphabet)
		{
			state_set next_set = epsilon_closure(
				move(current_set, c, nfa_trans), nfa_trans);

			if (next_set.empty())
				continue;

			std::string next_dfa_name;
			if (!dfa_state_names.contains(next_set))
			{
				next_dfa_name = name_from_set(next_set);
				dfa_state_names[next_set] = next_dfa_name;
				result.state_ids.insert(next_dfa_name);
				worklist.push(next_set);
			}
			else
			{
				next_dfa_name = dfa_state_names.at(next_set);
			}

			std::pair p = { current_dfa_name, c };
			result.transitions.emplace(p, next_dfa_name);
		}
	}

	result.current_state_id = result.initial_state_id;
	result.is_deterministic = true;

	return fsm::recognizer(std::move(result));
}

namespace details
{
template <typename T_Action>
state_machine_traits<recognizer>::output_type
recognize_internal(fsm::recognizer& recognizer, T_Action&& action)
{
	auto saved_state = recognizer.state();
	auto result = state_machine_traits<fsm::recognizer>::output_type{};

	try
	{
		result = std::forward<T_Action>(action)();
	}
	catch (...)
	{
	}

	recognizer = fsm::recognizer{ std::move(saved_state) };

	return result;
}
} // namespace details

template <std::convertible_to<state_machine_traits<recognizer>::output_type>... T_Args>
state_machine_traits<recognizer>::output_type
recognize(fsm::recognizer& recognizer, T_Args&&... args)
{
	return details::recognize_internal(recognizer, [&] {
		return recognizer.handle_input(std::forward<T_Args>(args)...);
	});
}

template <concepts::container T_Container>
state_machine_traits<recognizer>::output_type
recognize(fsm::recognizer& recognizer, T_Container&& inputs)
{
	return details::recognize_internal(recognizer, [&] {
		return recognizer.handle_input(inputs);
	});
}

} // namespace fsm

#endif // RECOGNIZER_HPP
