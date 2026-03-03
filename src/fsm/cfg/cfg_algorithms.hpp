#ifndef FSM_CFG_ALGORITHMS_H
#define FSM_CFG_ALGORITHMS_H

#include "../concepts.hpp"
#include "../default_symbol_generator.hpp"
#include "basic_cfg.hpp"

#include <algorithm>
#include <ranges>

namespace fsm
{
namespace algorithms::impl
{
class IsolateStartSymbol_fn
{
public:
	template <typename T_Symbol, typename T_Comp, typename T_Gen>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g, T_Gen& gen) const
	{
		auto result = g;
		auto new_start = gen.next_start_symbol(g.start_symbol(), g.non_terminals());

		result.add_non_terminal(new_start);
		result.add_rule({ new_start, { g.start_symbol() } });
		result.set_start_symbol(new_start);

		return result;
	}

	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		default_symbol_generator<T_Symbol> gen;

		return IsolateStartSymbol_fn::operator()(g, gen);
	}
};

class RemoveEpsilonRules_fn
{
public:
	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		auto nullables = find_nullable_symbols(g);

		auto result = g;
		result.set_rules(generate_new_rules(g, nullables));

		return result;
	}

private:
	template <typename T_Symbol>
	void generate_combinations(
		const std::vector<T_Symbol>& rhs,
		const std::set<T_Symbol>& nullables,
		const size_t index,
		std::vector<T_Symbol> current,
		std::set<std::vector<T_Symbol>>& out) const
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

	template <typename T_Grammar>
	std::set<typename T_Grammar::rule_type>
	generate_new_rules(
		const T_Grammar& grammar,
		std::set<typename T_Grammar::symbol_type> nullables) const
	{
		using rule_type = typename T_Grammar::rule_type;
		using symbol_type = typename T_Grammar::symbol_type;

		std::set<rule_type> new_rules;
		for (const auto& r : grammar.rules())
		{
			if (r.is_epsilon())
			{
				continue;
			}

			std::set<std::vector<symbol_type>> combinations;
			generate_combinations(r.rhs, nullables, 0, {}, combinations);

			for (const auto& comb : combinations)
			{
				if (!comb.empty() || r.lhs == grammar.start_symbol())
				{
					new_rules.insert({ r.lhs, comb });
				}
			}
		}

		return new_rules;
	}

	template <typename T_Symbol, typename T_Comp>
	std::set<T_Symbol> find_nullable_symbols(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		std::set<T_Symbol> nullables;
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& r : g.rules())
			{
				if (nullables.contains(r.lhs))
				{
					continue;
				}
				const bool is_now_nullable = r.is_epsilon()
					|| std::ranges::all_of(r.rhs, [&](const auto& s) {
						   return nullables.contains(s);
					   });

				if (is_now_nullable)
				{
					changed = nullables.insert(r.lhs).second;
				}
			}
		}

		return nullables;
	}
};

class RemoveUnitRules_fn
{
public:
	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		using grammar_type = basic_cfg<T_Symbol, T_Comp>;
		using rule_type = typename grammar_type::rule_type;
		using symbol_type = typename grammar_type::symbol_type;

		grammar_type result = g;
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

private:
	template <typename T_Grammar>
	static bool is_unit_rule(const typename T_Grammar::rule_type& r, const T_Grammar& g)
	{
		return r.rhs.size() == 1 && g.is_non_terminal(r.rhs[0]);
	}

	template <typename T_Symbol, typename T_Comp>
	std::map<T_Symbol, std::set<T_Symbol>>
	find_unit_pairs(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		std::map<T_Symbol, std::set<T_Symbol>> pairs;
		for (const auto& nt : g.non_terminals())
		{
			pairs[nt].insert(nt);
		}

		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& r : g.rules())
			{
				if (is_unit_rule(r, g))
				{
					for (auto& [lhs, targets] : pairs)
					{
						if (targets.contains(r.lhs))
						{
							changed |= targets.insert(r.rhs[0]).second;
						}
					}
				}
			}
		}

		return pairs;
	}
};

class RemoveUselessSymbols_fn
{
public:
	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		auto generating = find_generating(g);
		auto reachable = find_reachable(g, generating);

		return filter_grammar(g, generating, reachable);
	}

private:
	template <typename T_Symbol, typename T_Comp>
	std::set<T_Symbol> find_generating(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		std::set<T_Symbol> gen = g.terminals();
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& r : g.rules())
			{
				if (gen.contains(r.lhs))
				{
					continue;
				}
				if (std::ranges::all_of(r.rhs, [&](const auto& s) { return gen.contains(s); }))
				{
					changed |= gen.insert(r.lhs).second;
				}
			}
		}

		return gen;
	}

	template <typename T_Symbol, typename T_Comp>
	std::set<T_Symbol> find_reachable(
		const basic_cfg<T_Symbol, T_Comp>& g,
		const std::set<T_Symbol>& gen) const
	{
		std::set<T_Symbol> reach = { g.start_symbol() };
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& r : g.rules())
			{
				if (reach.contains(r.lhs)
					&& std::ranges::all_of(r.rhs, [&](const auto& s) { return gen.contains(s); }))
				{
					for (const auto& s : r.rhs)
					{
						changed |= reach.insert(s).second;
					}
				}
			}
		}

		return reach;
	}

	template <typename T_Symbol, typename T_Comp>
	basic_cfg<T_Symbol, T_Comp>
	filter_grammar(
		const basic_cfg<T_Symbol, T_Comp>& g,
		const std::set<T_Symbol>& gen,
		const std::set<T_Symbol>& reach) const
	{
		basic_cfg<T_Symbol, T_Comp> res;
		res.set_start_symbol(g.start_symbol());
		for (const auto& t : g.terminals())
		{
			if (reach.contains(t))
			{
				res.add_terminal(t);
			}
		}
		for (const auto& nt : g.non_terminals())
		{
			if (reach.contains(nt) && gen.contains(nt))
			{
				res.add_non_terminal(nt);
			}
		}
		for (const auto& r : g.rules())
		{
			const bool ok = reach.contains(r.lhs)
				&& std::ranges::all_of(r.rhs, [&](const auto& s) {
					   return reach.contains(s) && (g.is_terminal(s) || gen.contains(s));
				   });

			if (ok)
			{
				res.add_rule(r);
			}
		}

		return res;
	}
};

class MergeEquivalentSymbols_fn
{
public:
	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		auto result = g;
		while (true)
		{
			auto replacement_map = find_replacements(result);
			if (replacement_map.empty())
			{
				break;
			}
			result = apply_replacements(result, replacement_map);
		}

		return result;
	}

private:
	template <typename T_Symbol, typename T_Comp>
	std::map<T_Symbol, T_Symbol> find_replacements(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		std::map<T_Symbol, std::set<std::vector<T_Symbol>>> prods;
		for (const auto& r : g.rules())
		{
			prods[r.lhs].insert(r.rhs);
		}

		std::map<std::set<std::vector<T_Symbol>>, std::vector<T_Symbol>> groups;
		for (const auto& nt : g.non_terminals())
		{
			groups[prods[nt]].push_back(nt);
		}

		std::map<T_Symbol, T_Symbol> remap;
		for (const auto& [rhs_set, lhs_list] : groups)
		{
			if (lhs_list.size() <= 1)
			{
				continue;
			}
			T_Symbol survivor = lhs_list[0];
			for (const auto& s : lhs_list)
			{
				if (s == g.start_symbol())
				{
					survivor = s;
				}
			}
			for (const auto& v : lhs_list)
			{
				if (v != survivor)
				{
					remap[v] = survivor;
				}
			}
		}

		return remap;
	}

	template <typename T_Symbol, typename T_Comp>
	basic_cfg<T_Symbol, T_Comp> apply_replacements(
		const basic_cfg<T_Symbol, T_Comp>& g,
		const std::map<T_Symbol, T_Symbol>& remap) const
	{
		basic_cfg<T_Symbol, T_Comp> next;
		auto fix = [&](const T_Symbol& s) { return remap.contains(s) ? remap.at(s) : s; };
		next.set_start_symbol(fix(g.start_symbol()));

		for (const auto& t : g.terminals())
		{
			next.add_terminal(t);
		}
		for (const auto& nt : g.non_terminals())
		{
			if (!remap.contains(nt))
			{
				next.add_non_terminal(nt);
			}
		}
		for (const auto& r : g.rules())
		{
			if (remap.contains(r.lhs))
			{
				continue;
			}
			auto new_rhs = r.rhs;
			for (auto& s : new_rhs)
			{
				s = fix(s);
			}
			next.add_rule({ r.lhs, new_rhs });
		}

		return next;
	}
};

class ChomskyNormalForm_fn
{
public:
	template <typename T_Symbol, typename T_Comp, typename T_Gen>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g, T_Gen& gen) const
	{
		auto cnf = prepare_grammar(g, gen);
		auto current_rules = cnf.rules();
		cnf.clear_rules();

		std::map<T_Symbol, T_Symbol> term_to_nt{};
		for (const auto& r : current_rules)
		{
			process_rule(r, cnf, gen, term_to_nt);
		}
		return cnf;
	}

	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Comp>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g) const
	{
		default_symbol_generator<T_Symbol> gen;
		return operator()(g, gen);
	}

private:
	template <typename T_Grammar, typename T_Gen>
	T_Grammar prepare_grammar(const T_Grammar& g, T_Gen& gen) const
	{
		auto res = IsolateStartSymbol_fn{}(g, gen);
		res = RemoveEpsilonRules_fn{}(res);
		res = RemoveUnitRules_fn{}(res);

		return RemoveUselessSymbols_fn{}(res);
	}

	template <typename T_Rule, typename T_Grammar, typename T_Gen, typename T_Map>
	void process_rule(const T_Rule& r, T_Grammar& cnf, T_Gen& gen, T_Map& tmap) const
	{
		if (r.is_epsilon() || (r.rhs.size() == 1 && cnf.is_terminal(r.rhs[0])))
		{
			cnf.add_rule(r);
			return;
		}

		std::vector<typename T_Grammar::symbol_type> binarized;
		for (const auto& s : r.rhs)
		{
			if (cnf.is_terminal(s))
			{
				if (!tmap.contains(s))
				{
					auto nt = gen.next_terminal_proxy(s);
					cnf.add_non_terminal(nt);
					cnf.add_rule({ nt, { s } });
					tmap[s] = nt;
				}
				binarized.push_back(tmap[s]);
			}
			else
			{
				binarized.push_back(s);
			}
		}

		if (binarized.size() <= 2)
		{
			cnf.add_rule({ r.lhs, binarized });
			return;
		}

		auto curr_lhs = r.lhs;
		for (size_t i = 0; i + 2 < binarized.size(); ++i)
		{
			auto next_nt = gen.next_intermediate();
			cnf.add_non_terminal(next_nt);
			cnf.add_rule({ curr_lhs, { binarized[i], next_nt } });
			curr_lhs = next_nt;
		}
		cnf.add_rule({ curr_lhs, { binarized[binarized.size() - 2], binarized.back() } });
	}
};

template <typename T_Symbol>
struct ParseTreeNode
{
	T_Symbol symbol;
	std::vector<std::shared_ptr<ParseTreeNode>> children;

	[[nodiscard]] std::string to_string() const
	{
		if (children.empty())
		{
			return "ε";
		}
		std::stringstream ss;
		ss << "(" << symbol;
		for (const auto& child : children)
		{
			if (child->children.empty())
			{
				ss << " " << child->symbol;
			}
			else
			{
				ss << " " << child->to_string();
			}
		}
		ss << ")";

		return ss.str();
	}
};

template <typename T_Symbol>
struct CykResult
{
	bool is_valid = false;

	std::vector<std::shared_ptr<ParseTreeNode<T_Symbol>>> trees;

	// table[start_index][length] = set<non_terminal>
	std::vector<std::vector<std::set<T_Symbol>>> table;

	explicit operator bool() const { return is_valid; }
};

class Cyk_fn
{
public:
	template <typename T_Symbol, typename T_Comp>
	[[nodiscard]] CykResult<T_Symbol>
	operator()(const basic_cfg<T_Symbol, T_Comp>& g, const std::vector<T_Symbol>& word) const
	{
		CykResult<T_Symbol> res;
		const size_t n = word.size();
		res.table.resize(n, std::vector<std::set<T_Symbol>>(n + 1));

		if (n == 0)
		{
			return handle_empty_word(g);
		}

		auto [term_rules, non_term_rules] = index_rules(g);
		auto dp = run_cyk_core(word, term_rules, non_term_rules, res.table);

		const auto& S = g.start_symbol();
		if (dp[0][n].contains(S))
		{
			res.is_valid = true;
			res.trees = build_all_trees(0, n, S, word, dp);
		}

		return res;
	}

	template <concepts::is_string_like T_Str>
	[[nodiscard]] auto operator()(
		const basic_cfg<T_Str>& g,
		const std::type_identity_t<T_Str>& w) const
	{
		std::vector<T_Str> v;
		for (const auto c : w)
		{
			v.push_back(T_Str{ c });
		}

		return (*this)(g, v);
	}

private:
	template <typename T>
	struct Derivation
	{
		T B, C;
		size_t k;
	};

	template <typename T_Symbol>
	CykResult<T_Symbol> handle_empty_word(const basic_cfg<T_Symbol>& g) const
	{
		CykResult<T_Symbol> res;
		for (const auto& r : g.rules())
		{
			if (r.lhs == g.start_symbol() && r.is_epsilon())
			{
				res.is_valid = true;
				auto root = std::make_shared<ParseTreeNode<T_Symbol>>();
				root->symbol = r.lhs;
				res.trees.push_back(root);

				break;
			}
		}

		return res;
	}

	template <typename T_Symbol>
	auto index_rules(const basic_cfg<T_Symbol>& g) const
	{
		std::map<T_Symbol, std::set<T_Symbol>> t_map;
		std::map<std::pair<T_Symbol, T_Symbol>, std::set<T_Symbol>> nt_map;
		for (const auto& r : g.rules())
		{
			if (r.rhs.size() == 1)
			{
				t_map[r.rhs[0]].insert(r.lhs);
			}
			else if (r.rhs.size() == 2)
			{
				nt_map[{ r.rhs[0], r.rhs[1] }].insert(r.lhs);
			}
		}

		return std::make_pair(t_map, nt_map);
	}

	template <typename T_Symbol>
	auto run_cyk_core(const std::vector<T_Symbol>& word,
		const std::map<T_Symbol, std::set<T_Symbol>>& t_rules,
		const std::map<std::pair<T_Symbol, T_Symbol>, std::set<T_Symbol>>& nt_rules,
		std::vector<std::vector<std::set<T_Symbol>>>& out_table) const
	{
		std::size_t n = word.size();
		using Cell = std::map<T_Symbol, std::vector<Derivation<T_Symbol>>>;
		std::vector<std::vector<Cell>> dp(n, std::vector<Cell>(n + 1));

		for (std::size_t i = 0; i < n; ++i)
		{
			if (t_rules.contains(word[i]))
			{
				for (const auto& A : t_rules.at(word[i]))
				{
					dp[i][1][A].push_back({});
					out_table[i][1].insert(A);
				}
			}
		}

		for (std::size_t l = 2; l <= n; ++l)
		{
			for (std::size_t i = 0; i <= n - l; ++i)
			{
				for (std::size_t k = 1; k < l; ++k)
				{
					for (const auto& [B, _] : dp[i][k])
					{
						for (const auto& [C, _1] : dp[i + k][l - k])
						{
							if (auto it = nt_rules.find({ B, C }); it != nt_rules.end())
							{
								for (const auto& A : it->second)
								{
									dp[i][l][A].push_back({ B, C, k });
									out_table[i][l].insert(A);
								}
							}
						}
					}
				}
			}
		}

		return dp;
	}

	template <typename T_Symbol, typename T_DP>
	auto build_all_trees(
		std::size_t i, std::size_t l,
		T_Symbol A,
		const std::vector<T_Symbol>& word,
		const T_DP& dp) const
	{
		std::vector<std::shared_ptr<ParseTreeNode<T_Symbol>>> sub;
		if (l == 1)
		{
			auto r = std::make_shared<ParseTreeNode<T_Symbol>>(A);
			r->children.push_back(std::make_shared<ParseTreeNode<T_Symbol>>(word[i]));
			sub.push_back(r);
			return sub;
		}
		for (const auto& d : dp[i][l].at(A))
		{
			auto lefts = build_all_trees(i, d.k, d.B, word, dp);
			auto rights = build_all_trees(i + d.k, l - d.k, d.C, word, dp);
			for (auto& lt : lefts)
			{
				for (auto& rt : rights)
				{
					auto r = std::make_shared<ParseTreeNode<T_Symbol>>(A);
					r->children = { lt, rt };
					sub.push_back(r);
				}
			}
		}

		return sub;
	}
};

class PrintCykTable_fn
{
public:
	template <typename T_Symbol>
	void operator()(
		const CykResult<T_Symbol>& res,
		const std::vector<T_Symbol>& word,
		std::ostream& os = std::cout) const
	{ // TODO: Replace with templated ostream
		if (res.table.empty())
		{
			os << "Table is empty.\n";
			return;
		}

		const std::size_t n = word.size();
		std::size_t cell_w = calculate_width(res, word);

		os << "\nCYK Table (Rows: Length, Cols: Start Index):\n";
		for (size_t l = n; l >= 1; --l)
		{
			os << "Len " << std::setw(2) << l << " |";
			for (size_t i = 0; i <= n - l; ++i)
			{
				print_cell(os, res.table[i][l], cell_w);
			}
			os << "\n";
		}

		os << "       " << std::string((cell_w + 1) * n + 1, '-') << "\nWord:   ";
		for (const auto& w : word)
		{
			std::stringstream ss;
			ss << w;
			print_padded(os, ss.str(), cell_w);
		}
		os << "|\n";
	}

	void operator()(
		const CykResult<std::string>& r,
		const std::string& w,
		std::ostream& os = std::cout) const
	{ // TODO: Replace with templated string type
		std::vector<std::string> v;
		for (char c : w)
		{
			v.emplace_back(1, c);
		}

		(*this)(r, v, os);
	}

private:
	template <typename T_Symbol>
	[[nodiscard]] std::size_t calculate_width(
		const CykResult<T_Symbol>& res,
		const std::vector<T_Symbol>& word) const
	{
		std::size_t m = 3;
		for (auto& row : res.table)
			for (auto& s : row)
			{
				std::stringstream ss;
				ss << "{";
				for (auto it = s.begin(); it != s.end(); ++it)
				{
					ss << (it == s.begin() ? "" : ",") << *it;
				}
				ss << "}";
				m = std::max(m, ss.str().length());
			}
		for (auto& w : word)
		{
			std::stringstream ss;
			ss << w;
			m = std::max(m, ss.str().length() + 2);
		}
		return m + 2;
	}

	template <typename T_Char>
	static void print_padded(
		std::basic_ostream<T_Char>& os,
		const std::basic_string<T_Char>& s,
		const std::size_t w)
	{
		const std::size_t p = w - s.length();
		os << "|" << std::string(p / 2, ' ') << s << std::string(p - p / 2, ' ');
	}

	template <typename T_Char, typename T_Set>
	void print_cell(std::basic_ostream<T_Char>& os, const T_Set& s, const std::size_t w) const
	{
		std::stringstream ss;
		ss << "{";
		for (auto it = s.begin(); it != s.end(); ++it)
		{
			ss << (it == s.begin() ? "" : ",") << *it;
		}
		ss << "}";
		const std::string str = ss.str();
		const std::size_t p = w - str.length();
		os << std::string(p / 2, ' ') << str << std::string(p - p / 2, ' ') << "|";
	}
};

class ReduceGrammar_fn
{
public:
	template <typename T_Symbol, typename T_Cmp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Cmp>
	operator()(const basic_cfg<T_Symbol, T_Cmp>& grammar) const
	{
		auto result = grammar;

		// result = RemoveEpsilonRules_fn{}(result);
		result = RemoveUnitRules_fn{}(result);
		result = RemoveUselessSymbols_fn{}(result);
		result = MergeEquivalentSymbols_fn{}(result);

		return result;
	}
};

class RemoveLeftRecursion_fn
{
public:
	template <typename T_Symbol, typename T_Cmp, typename T_Gen>
	[[nodiscard]] basic_cfg<T_Symbol, T_Cmp>
	operator()(const basic_cfg<T_Symbol, T_Cmp>& grammar, T_Gen& gen) const
	{
		using symbol_type = T_Symbol;

		auto result = grammar;
		result.clear_rules();

		std::vector<symbol_type> non_terminals(
			grammar.non_terminals().begin(),
			grammar.non_terminals().end());

		std::map<symbol_type, std::vector<std::vector<symbol_type>>> current_rules;
		for (const auto& r : grammar.rules())
		{
			current_rules[r.lhs].push_back(r.rhs);
		}

		for (std::size_t i = 0; i < non_terminals.size(); ++i)
		{
			symbol_type Ai = non_terminals[i];

			for (std::size_t j = 0; j < i; ++j)
			{
				symbol_type Aj = non_terminals[j];
				std::vector<std::vector<symbol_type>> new_Ai_rules;

				for (const auto& rhs : current_rules[Ai])
				{
					if (!rhs.empty() && rhs[0] == Aj)
					{
						std::vector<symbol_type> gamma(rhs.begin() + 1, rhs.end());

						for (const auto& delta : current_rules[Aj])
						{
							std::vector<symbol_type> combined = delta;
							combined.insert(combined.end(), gamma.begin(), gamma.end());
							new_Ai_rules.push_back(combined);
						}
					}
					else
					{
						new_Ai_rules.push_back(rhs);
					}
				}
				current_rules[Ai] = std::move(new_Ai_rules);
			}

			std::vector<std::vector<symbol_type>> alphas;
			std::vector<std::vector<symbol_type>> betas;

			for (const auto& rhs : current_rules[Ai])
			{
				if (!rhs.empty() && rhs[0] == Ai)
				{
					// Ai -> Ai alpha
					alphas.push_back(std::vector<symbol_type>(rhs.begin() + 1, rhs.end()));
				}
				else
				{
					// Ai -> beta
					betas.push_back(rhs);
				}
			}

			if (!alphas.empty())
			{
				symbol_type Ai_prime = gen.next_intermediate();
				result.add_non_terminal(Ai_prime);

				std::vector<std::vector<symbol_type>> new_Ai_rules;

				if (betas.empty())
				{
					betas.push_back({});
				}

				// Ai -> beta Ai'
				for (const auto& beta : betas)
				{
					std::vector<symbol_type> new_rhs = beta;
					new_rhs.push_back(Ai_prime);
					new_Ai_rules.push_back(new_rhs);
				}
				current_rules[Ai] = std::move(new_Ai_rules);

				// Ai' -> alpha Ai' | ε
				for (const auto& alpha : alphas)
				{
					std::vector<symbol_type> new_rhs = alpha;
					new_rhs.push_back(Ai_prime);
					current_rules[Ai_prime].push_back(new_rhs);
				}
				current_rules[Ai_prime].push_back({});
			}
		}

		result.set_start_symbol(grammar.start_symbol());
		for (const auto& t : grammar.terminals())
		{
			result.add_terminal(t);
		}
		for (const auto& nt : grammar.non_terminals())
		{
			result.add_non_terminal(nt);
		}

		for (const auto& [lhs, rhs] : current_rules)
		{
			for (const auto& symbol : rhs)
			{
				result.add_rule({ lhs, symbol });
			}
		}

		return result;
	}

	template <typename T_Symbol, typename T_Cmp>
	[[nodiscard]] basic_cfg<T_Symbol, T_Cmp>
	operator()(const basic_cfg<T_Symbol, T_Cmp>& grammar) const
	{
		default_symbol_generator<T_Symbol> gen;
		return operator()(grammar, gen);
	}

private:
};

} // namespace algorithms::impl

namespace algorithms
{

template <typename T_Symbol>
using cyk_result = impl::CykResult<T_Symbol>;

template <typename T_Symbol>
using parse_tree_node = impl::ParseTreeNode<T_Symbol>;

inline constexpr impl::IsolateStartSymbol_fn isolate_start_symbol;

inline constexpr impl::RemoveEpsilonRules_fn remove_epsilon_rules;

inline constexpr impl::RemoveUnitRules_fn remove_unit_rules;

inline constexpr impl::RemoveUselessSymbols_fn remove_useless_symbols;

inline constexpr impl::MergeEquivalentSymbols_fn merge_equivalent_symbols;

inline constexpr impl::ReduceGrammar_fn reduce_grammar;

inline constexpr impl::RemoveLeftRecursion_fn remove_left_recursion;

inline constexpr impl::ChomskyNormalForm_fn to_chomsky_normal_form;

inline constexpr impl::Cyk_fn cyk;

inline constexpr impl::PrintCykTable_fn print_cyk_table;
} // namespace algorithms
} // namespace fsm

#endif // FSM_CFG_ALGORITHMS_H
