#ifndef FSM_LL1_TABLE_HPP
#define FSM_LL1_TABLE_HPP

#include "../cfg/basic_cfg.hpp"

#include <map>

namespace fsm::ll1
{

template <
	typename T_Symbol,
	typename T_Compare = std::less<T_Symbol>,
	typename T_Rule = cfg_rule<T_Symbol>>
class table
{
	using terminal_storage_t = std::map<T_Symbol, T_Rule, T_Compare>;
	using symbol_storage_t = std::map<T_Symbol, std::set<T_Symbol, T_Compare>, T_Compare>;
	using storage_t = std::map<T_Symbol, terminal_storage_t, T_Compare>;

public:
	using symbol_type = T_Symbol;
	using rule_type = T_Rule;

	table(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const T_Symbol& epsilon_symbol,
		const T_Symbol& end_marker)
		: m_epsilon_symbol(epsilon_symbol)
		, m_end_symbol(end_marker)
	{
		build_table(g, epsilon_symbol, end_marker);
	}

	const storage_t& entries()
	{
		return m_entries;
	}

	void add_entry(const T_Symbol& lhs, const T_Symbol& terminal, const rule_type& rule)
	{
		if (m_entries[lhs].contains(terminal))
		{
			if (!(m_entries[lhs][terminal] == rule))
			{ // Collision
				const std::string err
					= "Grammar is not LL(1)! Collision at ["
					+ std::string(lhs) + "]["
					+ std::string(terminal) + "].\n"
					+ "Rules: \n1) " + m_entries[lhs][terminal].lhs
					+ " -> ...\n2) " + rule.lhs + " -> ...";
				throw std::runtime_error(err);
			}
		}
		m_entries[lhs][terminal] = rule;
	}

	bool has_rule(const T_Symbol& lhs, const T_Symbol& terminal) const
	{
		auto it = m_entries.find(lhs);
		if (it == m_entries.end())
		{
			return false;
		}

		return it->second.contains(terminal);
	}

	const T_Rule& get_rule(const T_Symbol& lhs, const T_Symbol& terminal) const
	{
		return m_entries.at(lhs).at(terminal);
	}

	void print(std::ostream& os = std::cout) const
	{
		os << "--- LL(1) Parsing Table ---\n";
		for (const auto& [lhs, row] : m_entries)
		{
			for (const auto& [term, rule] : row)
			{
				os << "M[" << lhs << ", " << term << "] = " << rule.lhs << " -> ";
				if (rule.is_epsilon())
				{
					os << "ε";
				}
				else
				{
					for (const auto& s : rule.rhs)
					{
						os << s << " ";
					}
				}
				os << "\n";
			}
		}
	}

	static table create(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const T_Symbol& epsilon_symbol,
		const T_Symbol& end_marker)
	{
		return table(g, epsilon_symbol, end_marker);
	}

private:
	T_Symbol m_epsilon_symbol;
	T_Symbol m_end_symbol;
	storage_t m_entries;

	void build_table(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const T_Symbol& epsilon_symbol,
		const T_Symbol& end_marker)
	{
		auto first_sets = compute_first(g, epsilon_symbol);
		auto follow_sets = compute_follow(g, first_sets, epsilon_symbol, end_marker);

		for (const auto& rule : g.rules())
		{
			const auto& A = rule.lhs;
			auto first_alpha = compute_first_of_sequence(rule.rhs, first_sets, epsilon_symbol);

			for (const auto& a : first_alpha)
			{
				if (a != epsilon_symbol)
				{
					add_entry(A, a, rule);
				}
			}

			if (first_alpha.count(epsilon_symbol))
			{
				for (const auto& b : follow_sets.at(A))
				{
					add_entry(A, b, rule);
				}
			}
		}
	}

	static std::set<T_Symbol>
	compute_first_of_sequence(
		const std::vector<T_Symbol>& rhs,
		const symbol_storage_t& first,
		const T_Symbol& epsilon)
	{
		std::set<T_Symbol> result;
		bool all_derive_epsilon = true;

		for (const auto& sym : rhs)
		{
			if (!first.contains(sym))
			{
				result.insert(sym);
				all_derive_epsilon = false;
				break;
			}

			const auto& sym_first = first.at(sym);
			for (const auto& a : sym_first)
			{
				if (a != epsilon)
				{
					result.insert(a);
				}
			}

			if (!sym_first.contains(epsilon))
			{
				all_derive_epsilon = false;
				break;
			}
		}

		if (all_derive_epsilon || rhs.empty())
		{
			result.insert(epsilon);
		}

		return result;
	}

	static symbol_storage_t
	compute_first(const basic_cfg<T_Symbol>& g, T_Symbol epsilon)
	{
		symbol_storage_t first;

		for (const auto& term : g.terminals())
		{
			first[term] = { term };
		}
		first[epsilon] = { epsilon };

		for (const auto& nt : g.non_terminals())
		{
			first[nt] = {};
		}

		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& rule : g.rules())
			{
				size_t old_size = first[rule.lhs].size();

				auto rhs_first = compute_first_of_sequence(rule.rhs, first, epsilon);
				first[rule.lhs].insert(rhs_first.begin(), rhs_first.end());

				if (first[rule.lhs].size() > old_size)
				{
					changed = true;
				}
			}
		}

		return first;
	}

	static symbol_storage_t
	compute_follow(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const symbol_storage_t& first,
		T_Symbol epsilon,
		T_Symbol eof_marker)
	{
		symbol_storage_t follow;

		for (const auto& nt : g.non_terminals())
		{
			follow[nt] = {};
		}

		follow[g.start_symbol()].insert(eof_marker);

		bool changed = true;
		while (changed)
		{
			changed = false;
			for (const auto& rule : g.rules())
			{
				const auto& A = rule.lhs;

				for (std::size_t i = 0; i < rule.rhs.size(); ++i)
				{
					const auto& B = rule.rhs[i];

					if (g.is_non_terminal(B))
					{
						std::size_t old_size = follow[B].size();

						std::vector<T_Symbol> beta(rule.rhs.begin() + i + 1, rule.rhs.end());
						auto first_beta = compute_first_of_sequence(beta, first, epsilon);

						for (const auto& b : first_beta)
						{
							if (b != epsilon)
							{
								follow[B].insert(b);
							}
						}

						if (beta.empty() || first_beta.contains(epsilon))
						{
							for (const auto& a : follow[A])
							{
								follow[B].insert(a);
							}
						}

						if (follow[B].size() > old_size)
						{
							changed = true;
						}
					}
				}
			}
		}

		return follow;
	}
};

} // namespace fsm::ll1

#endif // FSM_LL1_TABLE_HPP
