#ifndef FSM_LL1_TABLE_BUILDER_HPP
#define FSM_LL1_TABLE_BUILDER_HPP

#include "../cfg/basic_cfg.hpp"
#include "table.hpp"

#include <concepts>
#include <functional>

namespace fsm::ll1
{

enum class collision_strategy
{
	throw_exception,
	keep_first,
	keep_last,
};

template <
	std::equality_comparable T_Symbol,
	typename T_Compare = std::less<T_Symbol>>
class table_builder
{
	using symbol_storage_t = std::map<T_Symbol, std::set<T_Symbol, T_Compare>, T_Compare>;
	using grammar_type = basic_cfg<T_Symbol, T_Compare>;

public:
	using warning_callback = std::function<void(const std::string&)>;
	using table_type = table<T_Symbol, T_Compare>;
	using rule_type = typename grammar_type::rule_type;

	struct settings
	{
		T_Symbol epsilon;
		T_Symbol end_marker;
		warning_callback warning_callback{};
		collision_strategy collision_strategy{};
	};

	explicit table_builder(const grammar_type& grammar)
		: m_grammar(grammar)
	{
	}

	table_builder(const grammar_type& grammar, const settings& settings)
		: m_grammar(grammar)
		, m_epsilon(settings.epsilon)
		, m_end_marker(settings.end_marker)
		, m_strategy(settings.collision_strategy)
		, m_warning_callback(settings.warning_callback)
	{
	}

	table_builder(const grammar_type& grammar, const T_Symbol& epsilon, const T_Symbol& end_marker)
		: m_grammar(grammar)
		, m_epsilon(epsilon)
		, m_end_marker(end_marker)
	{
	}

	table_builder& with_epsilon(const T_Symbol& epsilon)
	{
		m_epsilon = epsilon;

		return *this;
	}

	table_builder& with_end_marker(const T_Symbol& end_marker)
	{
		m_end_marker = end_marker;

		return *this;
	}

	table_builder& with_collision_strategy(const collision_strategy collision_strategy)
	{
		m_strategy = collision_strategy;

		return *this;
	}

	table_builder& on_warning(const warning_callback& warning_callback)
	{
		m_warning_callback = warning_callback;

		return *this;
	}

	template <typename T_Formatter = symbol_formatter<T_Symbol>>
	table_type build(const T_Formatter& formatter = {}) const
	{
		table_type result;
		result
			.set_epsilon(m_epsilon)
			.set_end_marker(m_end_marker);

		auto first_sets = compute_first(m_grammar, m_epsilon);
		auto follow_sets = compute_follow(m_grammar, first_sets, m_epsilon, m_end_marker);

		auto set_to_string = [&formatter](const std::set<T_Symbol, T_Compare>& s) {
			std::string res = "{ ";
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
			res += " }";
			return res;
		};

		for (const auto& rule : m_grammar.rules())
		{
			const auto& A = rule.lhs;
			auto first_alpha = compute_first_of_sequence(rule.rhs, first_sets, m_epsilon);

			auto try_add_entry = [&](const T_Symbol& terminal, const std::string& reason) {
				if (result.has_rule(A, terminal))
				{
					const auto& existing_rule = result.at(A, terminal);

					if (existing_rule == rule)
					{
						return;
					}

					const std::string detailed_err = std::format(
						"LL(1) Collision at M[{}][{}].\n"
						"1) {} -> {}\n"
						"2) {} -> {}\n\n"
						"--- Detailed conflict analysis ---\n"
						"Non-terminal: {}\n"
						"Conflicting token (Lookahead): {}\n\n"
						"FIRST({})  = {}\n"
						"FOLLOW({}) = {}\n\n"
						"The reason:\n{}\n",
						formatter(A), formatter(terminal),
						formatter(existing_rule.lhs), format_rhs(existing_rule.rhs, formatter, m_epsilon),
						formatter(rule.lhs), format_rhs(rule.rhs, formatter, m_epsilon),
						formatter(A), formatter(terminal),
						formatter(A), set_to_string(first_sets.at(A)),
						formatter(A), set_to_string(follow_sets.at(A)),
						reason);

					if (m_strategy == collision_strategy::throw_exception)
					{
						throw std::runtime_error(detailed_err);
					}
					if (m_strategy == collision_strategy::keep_first)
					{
						if (m_warning_callback)
						{
							m_warning_callback("[WARNING] Conflict ignored (keep_first):\n" + detailed_err);
						}

						return;
					}
					if (m_strategy == collision_strategy::keep_last)
					{
						if (m_warning_callback)
						{
							m_warning_callback("[WARNING] Rule overridden (keep_last):\n" + detailed_err);
						}
						result.set_entry(A, terminal, rule);

						return;
					}
				}

				result.add_entry(A, terminal, rule);
			};

			for (const auto& a : first_alpha)
			{
				if (a != m_epsilon)
				{
					try_add_entry(a, "Token '" + formatter(a) + "' IN FIRST(" + format_rhs(rule.rhs, formatter, m_epsilon) + ").");
				}
			}

			if (first_alpha.count(m_epsilon))
			{
				for (const auto& b : follow_sets.at(A))
				{
					std::string eps = formatter(m_epsilon);
					try_add_entry(b, "The right side of the rule outputs empty symbol (" + eps + "), and token '" + formatter(b) + "' IN FOLLOW(" + formatter(A) + ").");
				}
			}
		}

		return result;
	}

private:
	grammar_type m_grammar;
	T_Symbol m_epsilon;
	T_Symbol m_end_marker;

	collision_strategy m_strategy = collision_strategy::throw_exception;

	std::function<void(const std::string&)> m_warning_callback{};

	template <typename T_Formatter>
	static std::string
	format_rhs(
		const std::vector<T_Symbol>& rhs,
		const T_Formatter& formatter,
		const T_Symbol& epsilon)
	{
		if (rhs.empty())
		{
			return formatter(epsilon);
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

#endif // FSM_LL1_TABLE_BUILDER_HPP
