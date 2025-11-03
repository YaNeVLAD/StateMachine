#ifndef DOT_HPP
#define DOT_HPP

#include "concepts.hpp"

#include <format>
#include <labeled.hpp>
#include <mealy_machine.hpp>
#include <moore_machine.hpp>
#include <recognizer.hpp>

#include <iostream>

namespace fsm
{
template <concepts::state_machine T_StateMachine>
T_StateMachine dot(std::istream&);

template <concepts::state_machine T_StateMachine>
void dot(std::ostream&, T_StateMachine const&);

namespace details
{
static constexpr auto EDGE_REGEX{ "ABOBA" };
static constexpr auto NODE_REGEX{ "ABOBA" };

inline std::string _unquote(std::string const& str)
{
	if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
	{
		return str.substr(1, str.length() - 2);
	}
	return str;
}

inline std::string _quote(std::string const& s)
{
	return "\"" + s + "\"";
}

template <typename T_From, typename T_To, typename T_Value>
std::ostream&
print_edge(std::ostream& os, T_From const& from, T_To const& to, std::optional<T_Value> const& value)
{
	os << "    " << from
	   << " -> " << to;

	if (value.has_value())
	{
		os << " [label = " << _quote(*value) << "]";
	}

	os << ";\n";

	return os;
}

template <typename T_State, typename... T_Args>
std::ostream& print_node(std::ostream& os, T_State const& state_id, T_Args&&... args)
{
	os << "    " << state_id;

	if (sizeof...(T_Args) > 0)
	{
		os << " [";

		std::size_t n{ 0 };
		((os << args << (++n != sizeof...(T_Args) ? " " : "")), ...);

		os << "]";
	}

	os << ";\n";

	return os;
}
} // namespace details

template <>
inline recognizer dot<recognizer>(std::istream& is)
{
	return recognizer{ recognizer_state{} };
}

template <>
inline void dot(std::ostream& os, moore_machine const& machine)
{
	os << "digraph MooreMachine {\n";
	os << "    rankdir = LR;\n\n";

	for (auto const& state_id : machine.state().state_ids)
	{
		auto it = machine.state().outputs.find(state_id);
		if (it == machine.state().outputs.end())
		{
			throw std::runtime_error("Inconsistent Moore machine: No output for state " + state_id);
		}
		auto const& output = it->second;
		auto label = details::_quote(std::format("{} / {}", state_id, output));

		details::print_node(os,
			details::_quote(state_id),
			make_labeled<"label">(label));
	}
	os << "\n";

	for (auto const& [state_input, to_state] : machine.state().transitions)
	{
		auto const& [from_state, input] = state_input;

		details::print_edge(os,
			details::_quote(from_state),
			details::_quote(to_state),
			std::optional{ input });
	}

	os << "}\n";
}

template <>
inline void dot(std::ostream& os, mealy_machine const& machine)
{
	os << "digraph MealyMachine {\n";
	os << "    rankdir = LR;\n\n";

	for (auto const& state_id : machine.state().state_ids)
	{
		details::print_node(os, details::_quote(state_id));
	}
	os << "\n";

	for (auto const& [state_input, state_output] : machine.state().transitions)
	{
		auto const& [from_state, input] = state_input;
		auto const& [to_state, output] = state_output;

		details::print_edge(
			os,
			details::_quote(from_state),
			details::_quote(to_state),
			std::optional{ std::format("{} / {}", input, output) });
	}

	os << "}\n";
}

template <>
inline void dot(std::ostream& os, recognizer const& recognizer)
{
	auto const& state = recognizer.state();

	os << "digraph Recognizer {\n";
	os << "    rankdir = LR;\n\n";

	os << "    // Start state pointer\n";
	os << "    " << details::_quote(state.initial_state_id) << ";\n\n";

	os << "    // States\n";
	for (auto const& id : state.state_ids)
	{
		const bool is_final = state.final_state_ids.contains(id);
		std::string shape = is_final ? "doublecircle" : "circle";

		details::print_node(os << std::boolalpha, details::_quote(id),
			make_labeled<"final">(is_final),
			make_labeled<"shape">(shape));
	}
	os << "\n";

	os << "    // Transitions\n";
	for (auto const& [state_and_input, next_state] : state.transitions)
	{
		auto const& [from_id, input_opt] = state_and_input;
		auto const& to_id = next_state;

		details::print_edge(os,
			details::_quote(from_id),
			details::_quote(to_id),
			input_opt);
	}

	os << "}\n";
}
} // namespace fsm

#endif // DOT_HPP
