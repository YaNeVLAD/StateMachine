#ifndef FSM_SLR_TABLE_PRINTER_HPP
#define FSM_SLR_TABLE_PRINTER_HPP

#include "table.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace fsm::slr
{

template <typename T_Symbol, typename T_State>
std::string action_to_string(const action<T_Symbol, T_State>& act)
{
	using namespace fsm::slr::actions;
	// clang-format off
	if (is_error(act)) return "";
	if (is_accept(act)) return "accept";
	if (is_shift(act)) return "s" + std::to_string(as_shift(act).target_state);
	if (is_reduce(act)) return "r(" + as_reduce(act).rule.lhs + ")";
	// clang-format on

	return "?";
}

template <typename T_Symbol, typename T_Compare>
void print_table(const table<T_Symbol, T_Compare>& table, std::ostream& out = std::cout)
{
	std::set<T_Symbol> terminals;
	std::set<T_Symbol> non_terminals;

	for (const auto& [state, map] : table.action_table())
	{
		for (const auto& [sym, act] : map)
		{
			terminals.insert(sym);
		}
	}

	for (const auto& [state, map] : table.goto_table())
	{
		for (const auto& [sym, target] : map)
		{
			non_terminals.insert(sym);
		}
	}

	out << std::setw(8) << "State" << " | ";
	for (const auto& t : terminals)
	{
		out << std::setw(8) << t << " ";
	}
	out << " | ";
	for (const auto& nt : non_terminals)
	{
		out << std::setw(8) << nt << " ";
	}
	out << "\n"
		<< std::string(20 + (terminals.size() + non_terminals.size()) * 9, '-')
		<< "\n";

	size_t max_state = 0;
	if (!table.action_table().empty())
	{
		max_state = std::max(max_state, table.action_table().rbegin()->first);
	}
	if (!table.goto_table().empty())
	{
		max_state = std::max(max_state, table.goto_table().rbegin()->first);
	}

	for (size_t i = 0; i <= max_state; ++i)
	{
		out << std::setw(8) << i << " | ";

		for (const auto& t : terminals)
		{
			out << std::setw(8) << action_to_string(table.get_action(i, t)) << " ";
		}

		out << " | ";

		for (const auto& nt : non_terminals)
		{
			auto res = table.get_goto(i, nt);
			if (res)
				out << std::setw(8) << *res << " ";
			else
				out << std::setw(8) << "" << " ";
		}
		out << "\n";
	}
}
} // namespace fsm::slr

#endif // FSM_SLR_TABLE_PRINTER_HPP
