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
	}

	const storage_t& entries() const
	{
		return m_entries;
	}

	const T_Symbol& epsilon() const
	{
		return m_epsilon_symbol;
	}

	const T_Symbol& end_marker() const
	{
		return m_end_symbol;
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

	table& set_epsilon(const T_Symbol& epsilon)
	{
		m_epsilon_symbol = epsilon;

		return *this;
	}

	table& set_end_marker(const T_Symbol& end_marker)
	{
		m_end_symbol = end_marker;

		return *this;
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

	std::string create_error_message(
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

	std::string format_rhs(const std::vector<T_Symbol>& rhs, symbol_formatter<T_Symbol>& formatter) const
	{
		if (rhs.empty())
		{
			return formatter(m_epsilon_symbol);
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
};
} // namespace fsm::ll1

#endif // FSM_LL1_TABLE_HPP
