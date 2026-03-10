#ifndef FSM_LL1_TABLE_PRINTER_HPP
#define FSM_LL1_TABLE_PRINTER_HPP

#include "../symbol_formatter.hpp"
#include "table.hpp"

#include <sstream>

namespace fsm::ll1
{
enum class table_format
{
	rules_list,
	compiled_table
};

struct table_printer_settings
{
	std::string_view epsilon_str = "<epsilon>";
	std::string_view separator = " -> ";
	std::string_view table_header = "--- LL(1) Parsing Table ---";
	table_format format = table_format::rules_list;
};

namespace details
{
template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
void print_as_rules(
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

template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
void print_as_compiled_table(
	const table<T_Symbol, T_Compare>& table,
	std::ostream& os = std::cout,
	const table_printer_settings& settings = {},
	F formatter = {})
{
	using row_map_t = typename std::decay_t<decltype(table.entries())>::mapped_type;
	using rule_t = typename row_map_t::mapped_type;

	struct RuleInfo
	{
		rule_t rule;
		std::set<T_Symbol, T_Compare> lookaheads;
	};
	std::map<T_Symbol, std::vector<RuleInfo>, T_Compare> rules_info;

	for (const auto& [lhs, row] : table.entries())
	{
		for (const auto& [term, rule] : row)
		{
			auto& infos = rules_info[lhs];
			auto it = std::ranges::find_if(infos, [&](const RuleInfo& info) {
				return info.rule.rhs == rule.rhs;
			});
			if (it == infos.end())
			{
				infos.push_back({ rule, { term } });
			}
			else
			{
				it->lookaheads.insert(term);
			}
		}
	}

	int current_index = 0;
	std::map<T_Symbol, int, T_Compare> dispatch_start;
	std::map<T_Symbol, std::vector<std::vector<int>>, T_Compare> rule_indices;

	for (const auto& [lhs, infos] : rules_info)
	{
		dispatch_start[lhs] = current_index;
		rule_indices[lhs].resize(infos.size());

		for (std::size_t k = 0; k < infos.size(); ++k)
		{
			std::size_t rhs_size = infos[k].rule.rhs.size();
			rule_indices[lhs][k].resize(std::max(1ull, rhs_size));
			rule_indices[lhs][k][0] = current_index++;
		}

		for (std::size_t k = 0; k < infos.size(); ++k)
		{
			std::size_t rhs_size = infos[k].rule.rhs.size();
			if (rhs_size > 1)
			{
				for (std::size_t j = 1; j < rhs_size; ++j)
				{
					rule_indices[lhs][k][j] = current_index++;
				}
			}
		}
	}

	struct RowData
	{
		T_Symbol symbol;
		std::set<T_Symbol, T_Compare> lookaheads;
		std::optional<int> next_row;
		bool push_next_to_stack{};
		bool is_epsilon{};
		bool err{};
		bool end{};
		bool shift{};
	};

	std::vector<RowData> compiled_table(current_index);

	for (const auto& [lhs, infos] : rules_info)
	{
		for (std::size_t k = 0; k < infos.size(); ++k)
		{
			const auto& rule = infos[k].rule;
			const auto& guiding_set = infos[k].lookaheads;

			bool is_last_alt = (k == infos.size() - 1);

			if (rule.rhs.empty())
			{
				RowData& row = compiled_table[rule_indices[lhs][k][0]];
				row.is_epsilon = true;
				row.lookaheads = guiding_set;
				row.next_row = std::nullopt;
				row.err = is_last_alt;
				row.push_next_to_stack = false;
				row.end = true;
				row.shift = false;
			}
			else
			{
				for (std::size_t j = 0; j < rule.rhs.size(); ++j)
				{
					T_Symbol sym = rule.rhs[j];
					RowData& row = compiled_table[rule_indices[lhs][k][j]];
					row.symbol = sym;
					row.is_epsilon = false;

					bool sym_is_nt = dispatch_start.contains(sym);

					row.shift = !sym_is_nt;
					row.end = (j == rule.rhs.size() - 1);

					if (j == 0)
					{
						row.lookaheads = guiding_set;
						row.err = is_last_alt;
					}
					else
					{
						if (!sym_is_nt)
						{
							row.lookaheads = { sym };
						}
						row.err = true;
					}

					if (sym_is_nt)
					{
						row.next_row = dispatch_start[sym];
						row.push_next_to_stack = true;
					}
					else
					{
						row.next_row = row.end ? std::nullopt : std::optional<int>(rule_indices[lhs][k][j + 1]);
						row.push_next_to_stack = false;
					}
				}
			}
		}
	}

	os << settings.table_header << "\n\n";

	os << std::left
	   << std::setw(4) << "Idx" << " | "
	   << std::setw(12) << "Первый" << " | "
	   << std::setw(12) << "Символы" << " | "
	   << std::setw(20) << "Куда дальше" << " | "
	   << std::setw(4) << "Err" << " | "
	   << std::setw(6) << "Stack" << " | "
	   << std::setw(4) << "End" << " | "
	   << std::setw(5) << "Shift" << "\n";
	os << std::string(85, '-') << "\n";

	for (std::size_t i = 0; i < compiled_table.size(); ++i)
	{
		const auto& row = compiled_table[i];

		std::stringstream sym_ss;
		if (row.is_epsilon)
		{
			sym_ss << settings.epsilon_str;
		}
		else
		{
			sym_ss << formatter(row.symbol);
		}
		std::string sym_str = sym_ss.str();

		std::stringstream la_ss;
		bool first = true;
		for (const auto& la : row.lookaheads)
		{
			if (!first)
			{
				la_ss << ", ";
			}
			la_ss << formatter(la);
			first = false;
		}
		std::string la_str = la_ss.str();

		std::string next_str = row.next_row.has_value() ? std::to_string(*row.next_row) : "-";

		os << std::left
		   << std::setw(4) << i << " | "
		   << std::setw(12) << sym_str << " | "
		   << std::setw(12) << la_str << " | "
		   << std::setw(20) << next_str << " | "
		   << std::setw(4) << (row.err ? "1" : "0") << " | "
		   << std::setw(6) << (row.push_next_to_stack ? "1" : "0") << " | "
		   << std::setw(4) << (row.end ? "1" : "0") << " | "
		   << std::setw(5) << (row.shift ? "1" : "0") << "\n";
	}
	os << std::endl;
}
} // namespace details

template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
void print_table(
	const table<T_Symbol, T_Compare>& table,
	std::ostream& os = std::cout,
	const table_printer_settings& settings = {},
	F formatter = {})
{
	if (settings.format == table_format::rules_list)
	{
		details::print_as_rules(table, os, settings, formatter);
	}
	else
	{
		details::print_as_compiled_table(table, os, settings, formatter);
	}
}
} // namespace fsm::ll1

#endif // FSM_LL1_TABLE_PRINTER_HPP
