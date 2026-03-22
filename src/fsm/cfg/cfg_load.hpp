#ifndef FSM_LOAD_CFG_HPP
#define FSM_LOAD_CFG_HPP

#include "../concepts.hpp"
#include "../utility.hpp"
#include "basic_cfg.hpp"

#include <iostream>
#include <istream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fsm
{
/**
 * Expected format:
 * LHS -> RHS1 RHS2 ... | RHS_ALT ...
 * | ANOTHER_ALT ...
 */
template <
	concepts::is_std_string_constructible T_Symbol = std::string,
	typename T_Compare = std::less<T_Symbol>>
basic_cfg<T_Symbol, T_Compare>
cfg_load(std::istream& in = std::cin)
{
	using grammar_type = basic_cfg<T_Symbol, T_Compare>;
	using rule_type = typename grammar_type::rule_type;

	std::set<T_Symbol, T_Compare> non_terminals;
	std::set<T_Symbol, T_Compare> terminals;
	std::set<rule_type> rules;

	std::string start_symbol;

	std::string line;
	std::size_t line_num = 0;

	std::string current_lhs;
	while (std::getline(in, line))
	{
		line_num++;
		std::string clean_line = utility::trim(line);

		if (clean_line.empty() || clean_line[0] == '#')
		{
			continue;
		}

		std::string rhs_part;
		if (clean_line[0] == '|')
		{
			if (current_lhs.empty())
			{
				throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": '|' alternative without a preceding LHS rule");
			}
			rhs_part = clean_line.substr(1);
		}
		else
		{
			std::size_t arrow_pos = clean_line.find("->");
			if (arrow_pos == std::string::npos)
			{
				throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": missing '->' or leading '|'");
			}

			current_lhs = utility::trim(clean_line.substr(0, arrow_pos));
			rhs_part = clean_line.substr(arrow_pos + 2);

			if (current_lhs.empty())
			{
				throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": empty LHS");
			}

			non_terminals.insert(T_Symbol(current_lhs));
			if (start_symbol.empty())
			{
				start_symbol = current_lhs;
			}
		}

		std::stringstream ss_pipe(rhs_part);
		std::string segment;

		while (std::getline(ss_pipe, segment, '|'))
		{
			std::vector<T_Symbol> current_rhs;
			std::stringstream ss_space(segment);
			std::string symbol;

			while (ss_space >> symbol)
			{
				if (symbol == "ε" || symbol == "\\e")
				{
					continue;
				}
				current_rhs.emplace_back(T_Symbol(symbol));
			}

			rules.insert({ T_Symbol(current_lhs), current_rhs });
		}
	}

	for (const auto& [lhs, rhs] : rules)
	{
		for (const auto& sym : rhs)
		{
			if (!non_terminals.contains(sym))
			{
				terminals.insert(sym);
			}
		}
	}

	T_Symbol start = start_symbol.empty() ? T_Symbol() : T_Symbol(start_symbol);

	return grammar_type{ non_terminals, terminals, rules, start };
}
} // namespace fsm

#endif // FSM_LOAD_CFG_HPP