#ifndef FSM_LL1_TABLE_PRINTER_HPP
#define FSM_LL1_TABLE_PRINTER_HPP

#include "../symbol_formatter.hpp"
#include "table.hpp"

namespace fsm::ll1
{
struct table_printer_settings
{
	std::string_view epsilon_str = "<epsilon>";
	std::string_view separator = " -> ";
	std::string_view table_header = "--- LL(1) Parsing Table ---";
};

template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
void print_table(
	const table<T_Symbol, T_Compare>& table,
	std::ostream& os = std::cout,
	const table_printer_settings& settings = {},
	F formatter = {})
{
	os << settings.table_header << std::endl;
	for (const auto& [lhs, row] : table.entries())
	{
		for (const auto& [term, rule] : row)
		{
			os << "M[" << formatter(lhs) << ", " << formatter(term) << "] = "
			   << formatter(rule.lhs)
			   << settings.separator;

			if (rule.is_epsilon())
			{
				os << settings.epsilon_str;
			}
			else
			{
				for (const auto& s : rule.rhs)
				{
					os << formatter(s) << " ";
				}
			}
			os << std::endl;
		}
	}
}

} // namespace fsm::ll1

#endif // FSM_LL1_TABLE_PRINTER_HPP
