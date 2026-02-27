#ifndef FSM_LOAD_CFG_HPP
#define FSM_LOAD_CFG_HPP

#include "../utility.hpp"
#include "basic_cfg.hpp"

#include <iostream>
#include <istream>
#include <sstream>
#include <string>

namespace fsm
{
/**
 * Expected format: LHS -> RHS1 RHS2 ... | RHS_ALT ...
 */
inline basic_cfg<std::string> cfg_load(std::istream& in = std::cin)
{
	std::set<std::string> non_terminals;
	std::set<std::string> terminals;
	std::set<cfg_rule<std::string>> rules;
	std::string start_symbol;

	std::string line;
	std::size_t line_num = 0;

	while (std::getline(in, line))
	{
		line_num++;
		std::string clean_line = utility::trim(line);
		if (clean_line.empty() || clean_line[0] == '#')
		{
			continue;
		}

		std::size_t arrow_pos = clean_line.find("->");
		if (arrow_pos == std::string::npos)
		{
			throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": missing '->'");
		}

		std::string lhs = utility::trim(clean_line.substr(0, arrow_pos));
		std::string rhs_part = clean_line.substr(arrow_pos + 2);

		if (lhs.empty())
		{
			throw std::runtime_error("Syntax error at line " + std::to_string(line_num) + ": empty LHS");
		}

		non_terminals.insert(lhs);
		if (start_symbol.empty())
		{
			start_symbol = lhs;
		}

		std::stringstream ss_pipe(rhs_part);
		std::string segment;

		while (std::getline(ss_pipe, segment, '|'))
		{
			std::vector<std::string> current_rhs;
			std::stringstream ss_space(segment);
			std::string symbol;

			while (ss_space >> symbol)
			{
				if (symbol == "ε" || symbol == "\\e")
				{
					continue;
				}
				current_rhs.push_back(symbol);
			}

			rules.insert({ lhs, current_rhs });
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

	return { non_terminals, terminals, rules, start_symbol };
}
} // namespace fsm

#endif // FSM_LOAD_CFG_HPP
