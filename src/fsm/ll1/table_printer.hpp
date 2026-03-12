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
	compiled_table,
	detailed_table,
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

template <typename T_Symbol, typename T_Compare, typename F = symbol_formatter<T_Symbol>>
void print_as_detailed_table(
	const basic_cfg<T_Symbol, T_Compare>& g,
	const table<T_Symbol, T_Compare>& tbl,
	const T_Symbol& epsilon_symbol,
	const T_Symbol& eof_symbol,
	std::ostream& os = std::cout,
	F formatter = {})
{
	using rule_t = cfg_rule<T_Symbol>;

	struct RuleInfo
	{
		rule_t rule;
		std::set<T_Symbol, T_Compare> lookaheads;
	};

	std::vector<T_Symbol> ordered_lhs;
	std::map<T_Symbol, std::vector<RuleInfo>, T_Compare> rules_info;

	for (const auto& rule : g.rules())
	{
		if (!rules_info.contains(rule.lhs))
		{
			ordered_lhs.push_back(rule.lhs);
		}

		std::set<T_Symbol, T_Compare> la;
		if (tbl.entries().contains(rule.lhs))
		{
			for (const auto& [term, tbl_rule] : tbl.entries().at(rule.lhs))
			{
				if (tbl_rule == rule)
				{
					la.insert(term);
				}
			}
		}
		rules_info[rule.lhs].push_back({ rule, la });
	}

	int current_idx = 1;
	std::map<T_Symbol, std::vector<int>, T_Compare> disp_indices;
	std::map<T_Symbol, int, T_Compare> first_disp_idx;

	for (const auto& lhs : ordered_lhs)
	{
		first_disp_idx[lhs] = current_idx;
		for (std::size_t k = 0; k < rules_info[lhs].size(); ++k)
		{
			disp_indices[lhs].push_back(current_idx++);
		}
	}

	std::map<T_Symbol, std::vector<std::vector<int>>, T_Compare> body_indices;
	for (const auto& lhs : ordered_lhs)
	{
		body_indices[lhs].resize(rules_info[lhs].size());
		for (std::size_t k = 0; k < rules_info[lhs].size(); ++k)
		{
			const auto& rule = rules_info[lhs][k].rule;
			std::size_t rhs_size = rule.rhs.size();

			if (lhs == g.start_symbol())
			{
				++rhs_size;
			}
			if (rhs_size == 0)
			{
				rhs_size = 1;
			}

			body_indices[lhs][k].resize(rhs_size);
			for (std::size_t j = 0; j < rhs_size; ++j)
			{
				body_indices[lhs][k][j] = current_idx++;
			}
		}
	}

	auto to_yes_no = [](const bool v) { return v ? "yes" : "no"; };
	auto set_to_str = [&](const std::set<T_Symbol, T_Compare>& s) {
		std::string res;
		bool first = true;
		for (const auto& sym : s)
		{
			if (!first)
			{
				res += ", ";
			}
			res += formatter(sym);
			first = false;
		}
		return res;
	};

	os << "--- LL(1) Table (Detailed format) ---\n";
	os << std::left
	   << std::setw(4) << "№" << " | "
	   << std::setw(6) << "Н.м." << " | "
	   << std::setw(15) << "Символы" << " | "
	   << std::setw(4) << "ERR" << " | "
	   << std::setw(8) << "Переход" << " | "
	   << std::setw(6) << "Сдвиг" << " | "
	   << std::setw(5) << "Стек" << "\n";
	os << std::string(64, '-') << "\n";

	for (const auto& lhs : ordered_lhs)
	{
		for (std::size_t k = 0; k < rules_info[lhs].size(); ++k)
		{
			const int idx = disp_indices[lhs][k];
			bool is_last = (k == rules_info[lhs].size() - 1);
			int target_body = body_indices[lhs][k][0];

			os << std::left
			   << std::setw(4) << idx << " | "
			   << std::setw(6) << formatter(lhs) << " | "
			   << std::setw(15) << set_to_str(rules_info[lhs][k].lookaheads) << " | "
			   << std::setw(4) << to_yes_no(is_last) << " | "
			   << std::setw(8) << target_body << " | "
			   << std::setw(6) << "" << " | "
			   << std::setw(5) << "" << "\n";
		}
	}

	os << std::string(64, '-') << "\n";

	for (const auto& lhs : ordered_lhs)
	{
		for (std::size_t k = 0; k < rules_info[lhs].size(); ++k)
		{
			const auto& rule = rules_info[lhs][k].rule;
			std::vector<T_Symbol> rhs_to_print = rule.rhs;

			if (lhs == g.start_symbol())
			{
				rhs_to_print.push_back(eof_symbol);
			}

			if (rhs_to_print.empty())
			{
				const int idx = body_indices[lhs][k][0];
				os << std::left << std::setw(4) << idx << " | "
				   << std::setw(6) << formatter(epsilon_symbol) << " | "
				   << std::setw(15) << set_to_str(rules_info[lhs][k].lookaheads) << " | "
				   << std::setw(4) << "yes" << " | "
				   << std::setw(8) << "NULL" << " | "
				   << std::setw(6) << "no" << " | "
				   << std::setw(5) << "no" << "\n";
			}
			else
			{
				for (std::size_t j = 0; j < rhs_to_print.size(); ++j)
				{
					T_Symbol sym = rhs_to_print[j];
					const int idx = body_indices[lhs][k][j];
					bool is_nt = g.is_non_terminal(sym);
					const bool is_last = (j == rhs_to_print.size() - 1);

					std::string next_str = is_last ? "NULL" : std::to_string(idx + 1);
					if (is_nt)
					{
						next_str = std::to_string(first_disp_idx[sym]);
					}

					std::string sym_col_str;
					if (is_nt)
					{
						std::set<T_Symbol, T_Compare> nt_la;
						if (tbl.entries().contains(sym))
						{
							for (const auto& [t, r] : tbl.entries().at(sym))
								nt_la.insert(t);
						}
						sym_col_str = set_to_str(nt_la);
					}
					else
					{
						sym_col_str = formatter(sym);
					}

					os << std::left << std::setw(4) << idx << " | "
					   << std::setw(6) << formatter(sym) << " | "
					   << std::setw(15) << sym_col_str << " | "
					   << std::setw(4) << "yes" << " | "
					   << std::setw(8) << next_str << " | "
					   << std::setw(6) << to_yes_no(!is_nt) << " | "
					   << std::setw(5) << to_yes_no(is_nt) << "\n";
				}
			}
		}
	}
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
	else if (settings.format == table_format::compiled_table)
	{
		details::print_as_compiled_table(table, os, settings, formatter);
	}
}
} // namespace fsm::ll1

#endif // FSM_LL1_TABLE_PRINTER_HPP
