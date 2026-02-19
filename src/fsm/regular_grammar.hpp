#ifndef REGULAR_GRAMMAR_HPP
#define REGULAR_GRAMMAR_HPP

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "converter.hpp"
#include "recognizer.hpp"

namespace fsm
{
enum class regular_grammar_type
{
	LeftLinear,
	RightLinear
};

struct production_rule
{
	std::string non_terminal_from;
	std::optional<std::string> terminal;
	std::optional<std::string> non_terminal_to;
};

struct regular_grammar_state
{

	regular_grammar_type type = regular_grammar_type::RightLinear;
	std::set<std::string> non_terminals;
	std::set<std::string> terminals;
	std::string start_symbol;
	std::vector<production_rule> rules;
};

class regular_grammar
{
public:
	regular_grammar() = default;

	explicit regular_grammar(regular_grammar_state state)
		: m_state(std::move(state))
	{
	}

	[[nodiscard]] regular_grammar_state const& state()
	{
		return m_state;
	}

	[[nodiscard]] regular_grammar_type type() const
	{
		return m_state.type;
	}

	[[nodiscard]] std::string const& start_symbol() const
	{
		return m_state.start_symbol;
	}

	[[nodiscard]] std::set<std::string> const& non_terminals() const
	{
		return m_state.non_terminals;
	}

	[[nodiscard]] std::set<std::string> const& terminals() const
	{
		return m_state.terminals;
	}

	[[nodiscard]] std::vector<production_rule> const& rules() const
	{
		return m_state.rules;
	}

private:
	regular_grammar_state m_state;
};

namespace details
{
inline recognizer convert_rg(regular_grammar const& grammar)
{
	recognizer_state nfa;
	nfa.is_deterministic = false;

	const std::string FINAL_STATE = "__qF";

	nfa.state_ids = grammar.non_terminals();
	nfa.state_ids.insert(FINAL_STATE);

	nfa.initial_state_id = grammar.start_symbol();
	nfa.final_state_ids.insert(FINAL_STATE);

	for (const auto& [non_terminal_from, terminal, non_terminal_to] : grammar.rules())
	{
		const auto& A = non_terminal_from;
		const auto& a = terminal;
		const auto& B = non_terminal_to;

		if (a.has_value() && B.has_value()) // A -> aB
		{
			nfa.transitions.emplace(std::make_pair(A, a), B.value());
		}
		else if (a.has_value() && !B.has_value()) // A -> a
		{
			nfa.transitions.emplace(std::make_pair(A, a), FINAL_STATE);
		}
		else if (!a.has_value() && B.has_value()) // A -> B (unit rule)
		{
			nfa.transitions.emplace(std::make_pair(A, std::nullopt), B.value());
		}
		else // A -> e
		{
			if (A == nfa.initial_state_id)
			{
				nfa.final_state_ids.insert(nfa.initial_state_id);
			}
			else
			{
				nfa.transitions.emplace(std::make_pair(A, std::nullopt), FINAL_STATE);
			}
		}
	}
	nfa.current_state_id = nfa.initial_state_id;

	return recognizer{ std::move(nfa) };
}

inline recognizer convert_lg(regular_grammar const& grammar)
{
	recognizer_state nfa;
	nfa.is_deterministic = false;

	const std::string START_STATE = "__qS";

	nfa.state_ids = grammar.non_terminals();
	nfa.state_ids.insert(START_STATE);

	nfa.initial_state_id = START_STATE;
	nfa.final_state_ids.insert(grammar.start_symbol());

	for (const auto& [non_terminal_from, terminal, non_terminal_to] : grammar.rules())
	{
		const auto& A = non_terminal_from;
		const auto& a = terminal;
		const auto& B = non_terminal_to;

		if (a.has_value() && B.has_value()) // A -> Ba
		{
			nfa.transitions.emplace(std::make_pair(B.value(), a), A);
		}
		else if (a.has_value() && !B.has_value()) // A -> a
		{
			nfa.transitions.emplace(std::make_pair(START_STATE, a), A);
		}
		else if (!a.has_value() && B.has_value()) // A -> B (unit rule)
		{
			nfa.transitions.emplace(std::make_pair(B.value(), std::nullopt), A);
		}
		else // A -> e
		{
			if (A == grammar.start_symbol())
			{
				nfa.final_state_ids.insert(START_STATE);
			}
			else
			{
				nfa.transitions.emplace(std::make_pair(START_STATE, std::nullopt), A);
			}
		}
	}
	nfa.current_state_id = nfa.initial_state_id;

	return recognizer{ std::move(nfa) };
}
} // namespace details

template <>
struct converter<regular_grammar, recognizer>
{
	static recognizer operator()(regular_grammar const& grammar)
	{
		if (grammar.type() == regular_grammar_type::RightLinear)
		{
			return details::convert_rg(grammar);
		}

		return details::convert_lg(grammar);
	}
};

inline constexpr auto regular_grammar_to_recognizer = converter<regular_grammar, recognizer>();

namespace details
{
inline production_rule parse_rule(std::string const& str, regular_grammar_type type)
{
	production_rule rule;
	std::stringstream ss(str);
	std::string part;

	ss >> rule.non_terminal_from >> part; // A ->
	if (part != "->")
		throw std::runtime_error("Invalid rule format: " + str);
	if (ss.eof())
	{
		return rule;
	} // A -> e

	ss >> part;

	if (type == regular_grammar_type::RightLinear) // A -> aB or A -> a
	{
		if (part.length() == 1)
		{ // A -> a or A -> B
			if (std::isupper(part[0]))
			{
				rule.non_terminal_to = part;
			}
			else
			{
				rule.terminal = part;
			}
		}
		else
		{
			rule.terminal = std::string(1, part[0]);
			rule.non_terminal_to = part.substr(1);
		}
	}
	else // LeftLinear: A -> Ba or A -> a
	{
		if (part.length() == 1)
		{
			if (std::isupper(part[0]))
			{
				rule.non_terminal_to = part;
			}
			else
			{
				rule.terminal = part;
			}
		}
		else
		{
			rule.non_terminal_to = std::string(1, part[0]);
			rule.terminal = part.substr(1);
		}
	}
	return rule;
}
} // namespace details

inline regular_grammar load_grammar(std::istream& is)
{
	regular_grammar_state grammar_state;
	std::string line, key, value;
	auto type = regular_grammar_type::RightLinear;

	while (std::getline(is, line))
	{
		std::stringstream ss(line);
		ss >> key;

		if (key == "TYPE:")
		{
			ss >> value;
			if (value == "LEFT")
				type = regular_grammar_type::LeftLinear;
			else if (value == "RIGHT")
				type = regular_grammar_type::RightLinear;
			grammar_state.type = type;
		}
		else if (key == "NON-TERMINALS:")
		{
			while (ss >> value)
				grammar_state.non_terminals.emplace(value);
		}
		else if (key == "TERMINALS:")
		{
			while (ss >> value)
				grammar_state.terminals.emplace(value);
		}
		else if (key == "START:")
		{
			ss >> value;
			grammar_state.start_symbol = value;
		}
		else if (key == "RULES:")
		{
			while (std::getline(is, line))
			{
				if (line.empty())
				{
					continue;
				}
				grammar_state.rules.emplace_back(details::parse_rule(line, type));
			}
			break;
		}
	}

	return regular_grammar{ std::move(grammar_state) };
}

inline void save_grammar(std::ostream& os, regular_grammar const& grammar)
{
	os << "TYPE: " << (grammar.type() == regular_grammar_type::LeftLinear ? "LEFT" : "RIGHT") << "\n";

	os << "NON-TERMINALS:";
	for (auto const& s : grammar.non_terminals())
		os << " " << s;
	os << "\n";

	os << "TERMINALS:";
	for (auto const& s : grammar.terminals())
		os << " " << s;
	os << "\n";

	os << "START: " << grammar.start_symbol() << "\n";

	os << "RULES:\n";
	for (const auto& [non_terminal_from, terminal, non_terminal_to] : grammar.rules())
	{
		os << non_terminal_from << " ->";
		if (grammar.type() == regular_grammar_type::RightLinear)
		{
			if (terminal)
				os << " " << terminal.value();
			if (non_terminal_to)
				os << (terminal ? "" : " ") << non_terminal_to.value();
		}
		else
		{
			if (non_terminal_to)
				os << " " << non_terminal_to.value();
			if (terminal)
				os << (non_terminal_to ? "" : " ") << terminal.value();
		}
		os << "\n";
	}
}
} // namespace fsm

#endif // REGULAR_GRAMMAR_HPP
