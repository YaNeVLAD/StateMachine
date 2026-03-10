#ifndef FSM_BASIC_CFG_HPP
#define FSM_BASIC_CFG_HPP

#include <set>
#include <vector>

namespace fsm
{
template <typename T_Symbol>
struct cfg_rule
{
	T_Symbol lhs;
	std::vector<T_Symbol> rhs;

	[[nodiscard]] bool is_epsilon() const
	{
		return rhs.empty();
	}

	template <typename T_OtherSymbol>
	bool operator<(const cfg_rule<T_OtherSymbol>& other) const
	{
		if (lhs != other.lhs)
		{
			return lhs < other.lhs;
		}

		return rhs < other.rhs;
	}

	template <typename T_OtherSymbol>
	bool operator==(const cfg_rule<T_OtherSymbol>& other) const
	{
		return lhs == other.lhs && rhs == other.rhs;
	}
};

template <
	typename T_Symbol,
	typename T_Comparator = std::less<T_Symbol>>
class basic_cfg final
{
	using symbol_storage_t = std::set<T_Symbol, T_Comparator>;

public:
	using symbol_type = T_Symbol;
	using rule_type = cfg_rule<T_Symbol>;

	basic_cfg() = default;

	basic_cfg(
		symbol_storage_t non_terminals,
		symbol_storage_t terminals,
		std::set<rule_type> rules,
		T_Symbol start_symbol)
		: m_non_terminals(std::move(non_terminals))
		, m_terminals(std::move(terminals))
		, m_rules(std::move(rules))
		, m_start_symbol(std::move(start_symbol))
	{
	}

	const symbol_storage_t& non_terminals() const
	{
		return m_non_terminals;
	}

	const symbol_storage_t& terminals() const
	{
		return m_terminals;
	}

	const std::set<rule_type>& rules() const
	{
		return m_rules;
	}

	const symbol_type& start_symbol() const
	{
		return m_start_symbol;
	}

	void set_start_symbol(symbol_type new_start)
	{
		m_start_symbol = std::move(new_start);
	}

	void add_non_terminal(const symbol_type& new_non_terminal)
	{
		m_non_terminals.insert(new_non_terminal);
	}

	void add_terminal(const symbol_type& new_terminal)
	{
		m_terminals.insert(new_terminal);
	}

	void add_rule(const rule_type& new_rule)
	{
		m_rules.insert(new_rule);
		m_non_terminals.insert(new_rule.lhs);
	}

	void remove_rule(const rule_type& rule_to_remove)
	{
		m_rules.erase(rule_to_remove);
	}

	void clear_rules()
	{
		m_rules.clear();
	}

	void set_rules(std::set<rule_type> new_rules)
	{
		m_rules = std::move(new_rules);
	}

	[[nodiscard]] bool is_terminal(const symbol_type& symbol) const
	{
		return m_terminals.contains(symbol);
	}

	[[nodiscard]] bool is_non_terminal(const symbol_type& symbol) const
	{
		return m_non_terminals.contains(symbol);
	}

	void print(std::ostream& os = std::cout) const
	{
		os << "Start: " << m_start_symbol << "\nRules:\n";

		if (m_rules.empty())
			return;

		auto it = m_rules.begin();
		while (it != m_rules.end())
		{
			const auto& current_lhs = it->lhs;
			os << "  " << current_lhs << " -> ";

			bool first_alternative = true;

			while (it != m_rules.end() && it->lhs == current_lhs)
			{
				if (!first_alternative)
				{
					os << " | ";
				}

				if (it->is_epsilon())
				{
					os << "ε";
				}
				else
				{
					for (std::size_t i = 0; i < it->rhs.size(); ++i)
					{
						os << it->rhs[i] << (i + 1 < it->rhs.size() ? " " : "");
					}
				}

				first_alternative = false;
				++it;
			}
			os << "\n";
		}
	}

private:
	symbol_storage_t m_non_terminals;
	symbol_storage_t m_terminals;
	std::set<rule_type> m_rules;
	symbol_type m_start_symbol;
};
} // namespace fsm

#endif // FSM_BASIC_CFG_HPP
