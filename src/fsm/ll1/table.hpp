#ifndef FSM_LL1_TABLE_HPP
#define FSM_LL1_TABLE_HPP

#include "../cfg/basic_cfg.hpp"
#include "../concepts.hpp"
#include "../symbol_formatter.hpp"

#include <map>

namespace fsm::ll1
{
template <
	std::equality_comparable T_Symbol,
	typename T_Compare = std::less<T_Symbol>>
class table final
{
	using terminal_storage_t = std::map<T_Symbol, cfg_rule<T_Symbol>, T_Compare>;
	using symbol_storage_t = std::map<T_Symbol, std::set<T_Symbol, T_Compare>, T_Compare>;
	using storage_t = std::map<T_Symbol, terminal_storage_t, T_Compare>;

public:
	using symbol_type = T_Symbol;
	using rule_type = cfg_rule<T_Symbol>;

	using iterator = typename storage_t::iterator;
	using const_iterator = typename storage_t::const_iterator;

	table() = default;

	table(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const T_Symbol& epsilon_symbol,
		const T_Symbol& end_marker)
		: m_epsilon_symbol(epsilon_symbol)
		, m_end_symbol(end_marker)
	{
		build_table(g, epsilon_symbol, end_marker);
	}

	const storage_t& entries() const
	{
		return m_entries;
	}

	table& add_entry(const T_Symbol& lhs, const T_Symbol& terminal, const rule_type& rule)
	{
		auto& row = m_entries[lhs];

		auto [it, inserted] = row.try_emplace(terminal, rule);

		if (!inserted)
		{
			const auto& existing_rule = it->second;

			if (!(existing_rule == rule))
			{ // Collision
				auto err_msg = create_error_message(lhs, terminal, rule, it->second);
				throw std::runtime_error(err_msg);
			}
		}

		return *this;
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

	const rule_type& at(const T_Symbol& lhs, const T_Symbol& terminal) const
	{
		try
		{
			return m_entries.at(lhs).at(terminal);
		}
		catch (...)
		{
			throw std::out_of_range("LL1 Table: No rule for given non-terminal and terminal");
		}
	}

	const rule_type& operator[](const T_Symbol& lhs, const T_Symbol& terminal) const
	{
		return at(lhs, terminal);
	}

	std::optional<std::reference_wrapper<const rule_type>>
	find(const T_Symbol& lhs, const T_Symbol& terminal) const
	{
		auto row_it = m_entries.find(lhs);
		if (row_it == m_entries.end())
		{
			return std::nullopt;
		}

		auto rule_it = row_it->second.find(terminal);
		if (rule_it == row_it->second.end())
		{
			return std::nullopt;
		}

		return std::cref(rule_it->second);
	}

	static table create(
		const basic_cfg<T_Symbol, T_Compare>& g,
		const T_Symbol& epsilon_symbol,
		const T_Symbol& end_marker)
	{
		return table(g, epsilon_symbol, end_marker);
	}

	auto begin()
	{
		return m_entries.begin();
	}

	auto end()
	{
		return m_entries.end();
	}

	auto begin() const
	{
		return m_entries.begin();
	}

	auto end() const
	{
		return m_entries.end();
	}

	[[nodiscard]] bool empty() const
	{
		return m_entries.empty();
	}

private:
	T_Symbol m_epsilon_symbol;
	T_Symbol m_end_symbol;
	storage_t m_entries;

	static std::string
	create_error_message(
		const T_Symbol& lhs,
		const T_Symbol& terminal,
		const rule_type& rule,
		const rule_type& existing_rule)
	{
		symbol_formatter<T_Symbol> formatter;

		std::string lhs_str = formatter(lhs);
		std::string terminal_str = formatter(terminal);

		std::string err_msg = std::format(
			"LL(1) Collision at M[{}][{}].\n"
			"1) {} -> {}\n"
			"2) {} -> {}\n",
			lhs_str, terminal_str,
			formatter(existing_rule.lhs), format_rhs(existing_rule.rhs, formatter),
			formatter(rule.lhs), format_rhs(rule.rhs, formatter));

		if constexpr (!std::formattable<T_Symbol, char> && !concepts::streamable<T_Symbol>)
		{
			err_msg += "(Note: To see actual symbol values, implement std::formatter or operator<< for T_Symbol)";
		}

		return err_msg;
	}

	static std::string format_rhs(const std::vector<T_Symbol>& rhs, symbol_formatter<T_Symbol>& formatter)
	{
		if (rhs.empty())
		{
			return "ε";
		}

		std::string result;
		for (size_t i = 0; i < rhs.size(); ++i)
		{
			result += formatter(rhs[i]);
			if (i + 1 < rhs.size())
			{
				result += " ";
			}
		}

		return result;
	}

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
