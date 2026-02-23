#ifndef FSM_CFG_ALGORITHMS_H
#define FSM_CFG_ALGORITHMS_H

#include "../default_symbol_generator.hpp"

#include <ranges>

namespace fsm
{
namespace details
{
template <typename T_Symbol>
void generate_combinations(const std::vector<T_Symbol>& rhs,
	const std::set<T_Symbol>& nullables,
	const size_t index,
	std::vector<T_Symbol> current,
	std::set<std::vector<T_Symbol>>& out)
{
	if (index == rhs.size())
	{
		out.insert(current);
		return;
	}

	current.push_back(rhs[index]);
	generate_combinations(rhs, nullables, index + 1, current, out);
	current.pop_back();

	if (nullables.contains(rhs[index]))
	{
		generate_combinations(rhs, nullables, index + 1, current, out);
	}
}
} // namespace details

namespace algorithms
{
template <typename T_Grammar, typename T_Gen>
T_Grammar isolate_start_symbol(const T_Grammar& g, T_Gen& gen)
{
	T_Grammar result = g;
	auto new_start = gen.next_start_symbol(g.start_symbol(), g.non_terminals());

	result.add_non_terminal(new_start);
	result.add_rule({ new_start, { g.start_symbol() } });
	result.set_start_symbol(new_start);

	return result;
}

template <typename T_Grammar>
T_Grammar isolate_start_symbol(const T_Grammar& g)
{
	default_symbol_generator<typename T_Grammar::symbol_type> gen;
	return isolate_start_symbol(g, gen);
}

template <typename T_Grammar>
T_Grammar remove_epsilon_rules(const T_Grammar& g)
{
	using rule_type = typename T_Grammar::rule_type;
	using symbol_type = typename T_Grammar::symbol_type;

	T_Grammar result = g;
	std::set<symbol_type> nullables;
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& r : result.rules())
		{
			if (!nullables.contains(r.lhs))
			{
				if (r.is_epsilon())
				{
					nullables.insert(r.lhs);
					changed = true;
				}
				else
				{
					bool all_nullable = true;
					for (const auto& sym : r.rhs)
					{
						if (!nullables.contains(sym))
						{
							all_nullable = false;
							break;
						}
					}
					if (all_nullable)
					{
						nullables.insert(r.lhs);
						changed = true;
					}
				}
			}
		}
	}

	std::set<rule_type> new_rules;
	for (const auto& r : result.rules())
	{
		if (r.is_epsilon())
		{
			continue;
		}

		std::set<std::vector<symbol_type>> combinations;
		details::generate_combinations(r.rhs, nullables, 0, {}, combinations);

		for (const auto& comb : combinations)
		{
			if (!comb.empty() || r.lhs == result.start_symbol())
			{
				new_rules.insert({ r.lhs, comb });
			}
		}
	}
	result.set_rules(new_rules);
	return result;
}

template <typename T_Grammar>
T_Grammar remove_unit_rules(const T_Grammar& g)
{
	using rule_type = typename T_Grammar::rule_type;
	using symbol_type = typename T_Grammar::symbol_type;

	T_Grammar result = g;
	auto rules = result.rules();
	std::map<symbol_type, std::set<symbol_type>> unit_pairs;
	for (const auto& nt : result.non_terminals())
	{
		unit_pairs[nt].insert(nt);
	}

	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& r : rules)
		{
			if (r.rhs.size() == 1 && result.is_non_terminal(r.rhs[0]))
			{
				const symbol_type& A = r.lhs;
				const symbol_type& B = r.rhs[0];

				for (auto& targets : unit_pairs | std::views::values)
				{
					if (targets.contains(A) && !targets.contains(B))
					{
						targets.insert(B);
						changed = true;
					}
				}
			}
		}
	}

	std::set<rule_type> new_rules;
	for (const auto& [A, targets] : unit_pairs)
	{
		for (const auto& B : targets)
		{
			for (const auto& r : rules)
			{
				if (r.lhs == B && !(r.rhs.size() == 1 && result.is_non_terminal(r.rhs[0])))
				{
					new_rules.insert({ A, r.rhs });
				}
			}
		}
	}
	result.set_rules(new_rules);

	return result;
}

template <typename T_Grammar>
T_Grammar remove_useless_symbols(const T_Grammar& g)
{
	using rule_type = typename T_Grammar::rule_type;
	using symbol_type = typename T_Grammar::symbol_type;

	std::set<symbol_type> generating = g.terminals();
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& r : g.rules())
		{
			if (!generating.contains(r.lhs))
			{
				bool all_gen = true;
				for (const auto& sym : r.rhs)
				{
					if (!generating.contains(sym))
					{
						all_gen = false;
						break;
					}
				}
				if (all_gen)
				{
					generating.insert(r.lhs);
					changed = true;
				}
			}
		}
	}

	std::set<rule_type> gen_rules;
	for (const auto& r : g.rules())
	{
		bool ok = generating.contains(r.lhs);
		for (const auto& sym : r.rhs)
			if (!generating.contains(sym))
				ok = false;
		if (ok)
			gen_rules.insert(r);
	}

	std::set<symbol_type> reachable = { g.start_symbol() };
	changed = true;
	while (changed)
	{
		changed = false;
		for (const auto& r : gen_rules)
		{
			if (reachable.contains(r.lhs))
			{
				for (const auto& sym : r.rhs)
				{
					if (!reachable.contains(sym))
					{
						reachable.insert(sym);
						changed = true;
					}
				}
			}
		}
	}

	T_Grammar result;
	result.set_start_symbol(g.start_symbol());
	for (const auto& t : g.terminals())
	{
		if (reachable.contains(t))
		{
			result.add_terminal(t);
		}
	}

	for (const auto& nt : g.non_terminals())
	{
		if (reachable.contains(nt) && generating.contains(nt))
		{
			result.add_non_terminal(nt);
		}
	}

	for (const auto& r : gen_rules)
	{
		bool ok = reachable.contains(r.lhs);
		for (const auto& sym : r.rhs)
		{
			if (!reachable.contains(sym))
			{
				ok = false;
			}
		}
		if (ok)
		{
			result.add_rule(r);
		}
	}

	return result;
}

template <typename T_Grammar, typename T_Gen>
T_Grammar to_chomsky_normal_form(const T_Grammar& original_g, T_Gen& gen)
{
	using rule_type = typename T_Grammar::rule_type;
	using symbol_type = typename T_Grammar::symbol_type;

	T_Grammar g = isolate_start_symbol(original_g, gen);
	g = remove_epsilon_rules(g);
	g = remove_unit_rules(g);
	g = remove_useless_symbols(g);

	T_Grammar cnf = g;
	std::set<rule_type> current_rules = cnf.rules();
	cnf.clear_rules();

	std::map<symbol_type, symbol_type> term_to_nt;

	for (const auto& r : current_rules)
	{
		if (r.is_epsilon())
		{
			cnf.add_rule(r);
			continue;
		}

		if (r.rhs.size() == 1 && cnf.is_terminal(r.rhs[0]))
		{
			cnf.add_rule(r);
			continue;
		}

		std::vector<symbol_type> binarized_rhs;
		for (const auto& sym : r.rhs)
		{
			if (cnf.is_terminal(sym))
			{
				if (!term_to_nt.contains(sym))
				{
					symbol_type new_nt = gen.next_terminal_proxy(sym);
					term_to_nt[sym] = new_nt;
					cnf.add_non_terminal(new_nt);
					cnf.add_rule({ new_nt, { sym } });
				}
				binarized_rhs.push_back(term_to_nt[sym]);
			}
			else
			{
				binarized_rhs.push_back(sym);
			}
		}

		if (binarized_rhs.size() == 1)
		{
			cnf.add_rule({ r.lhs, { binarized_rhs[0] } });
			continue;
		}

		symbol_type current_lhs = r.lhs;
		for (size_t i = 0; i + 2 < binarized_rhs.size(); ++i)
		{
			symbol_type next_nt = gen.next_intermediate();
			cnf.add_non_terminal(next_nt);
			cnf.add_rule({ current_lhs, { binarized_rhs[i], next_nt } });
			current_lhs = next_nt;
		}

		cnf.add_rule({ current_lhs, { binarized_rhs[binarized_rhs.size() - 2], binarized_rhs.back() } });
	}

	return cnf;
}

template <typename T_Grammar>
T_Grammar to_chomsky_normal_form(const T_Grammar& original_g)
{
	default_symbol_generator<typename T_Grammar::symbol_type> gen;
	return to_chomsky_normal_form(original_g, gen);
}

template <typename T_Grammar>
bool cyk(const T_Grammar& cnf_grammar, const std::vector<typename T_Grammar::symbol_type>& word)
{
	using symbol_type = typename T_Grammar::symbol_type;

	const size_t n = word.size();

	if (n == 0)
	{
		for (const auto& r : cnf_grammar.rules())
		{
			if (r.lhs == cnf_grammar.start_symbol() && r.is_epsilon())
			{
				return true;
			}
		}
		return false;
	}

	// terminal_rules: a -> {A, B...}
	std::map<symbol_type, std::set<symbol_type>> terminal_rules;
	// non_terminal_rules: (B, C) -> {A, S...}
	std::map<std::pair<symbol_type, symbol_type>, std::set<symbol_type>> non_terminal_rules;

	for (const auto& r : cnf_grammar.rules())
	{
		if (r.rhs.size() == 1)
		{
			terminal_rules[r.rhs[0]].insert(r.lhs);
		}
		else if (r.rhs.size() == 2)
		{
			non_terminal_rules[{ r.rhs[0], r.rhs[1] }].insert(r.lhs);
		}
	}

	// table[start_index][length] = set<non_terminal>
	std::vector<std::vector<std::set<symbol_type>>> table(
		n, std::vector<std::set<symbol_type>>(n + 1));

	for (size_t i = 0; i < n; ++i)
	{
		if (terminal_rules.contains(word[i]))
		{
			table[i][1] = terminal_rules.at(word[i]);
		}
	}

	for (size_t l = 2; l <= n; ++l)
	{
		for (size_t i = 0; i <= n - l; ++i)
		{
			for (size_t k = 1; k < l; ++k)
			{
				const auto& left_set = table[i][k];
				const auto& right_set = table[i + k][l - k];

				for (const auto& B : left_set)
				{
					for (const auto& C : right_set)
					{
						std::pair<symbol_type, symbol_type> bc_pair = { B, C };
						if (non_terminal_rules.contains(bc_pair))
						{
							const auto& lhs_set = non_terminal_rules.at(bc_pair);
							table[i][l].insert(lhs_set.begin(), lhs_set.end());
						}
					}
				}
			}
		}
	}

	return table[0][n].contains(cnf_grammar.start_symbol());
}
} // namespace algorithms
} // namespace fsm

#endif // FSM_CFG_ALGORITHMS_H
