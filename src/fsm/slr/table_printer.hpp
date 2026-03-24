#ifndef FSM_SLR_TABLE_PRINTER_HPP
#define FSM_SLR_TABLE_PRINTER_HPP

#include "table.hpp"

#include <iomanip>
#include <iostream>
#include <string>

namespace fsm::slr
{
template <typename T_Symbol, typename T_Compare>
void print_table(const slr::table<T_Symbol, T_Compare>& table, std::ostream& out = std::cout)
{
	using table_type = slr::table<T_Symbol, T_Compare>;
	using action_type = typename table_type::action_type;
	using symbol_type = typename table_type::symbol_type;
	using state_type = typename table_type::state_type;

	std::set<symbol_type> terminals;
	std::set<symbol_type> non_terminals;

	const auto to_string = [](const action_type& action) -> std::string {
		using namespace fsm::slr::actions;
		return utility::overloaded_visitor(
			action,
			[](const action_error&) -> std::string { return ""; },
			[](const action_accept&) -> std::string { return "accept"; },
			[](const action_reduce<symbol_type>& act) -> std::string { return "r(" + act.rule.lhs + ")"; },
			[](const action_shift<state_type>& act) -> std::string { return "s(" + std::to_string(act.target_state) + ")"; });
	};

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
			out << std::setw(8) << to_string(table.get_action(i, t)) << " ";
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
