#ifndef FSM_SLR_TABLE_BUILDER_HPP
#define FSM_SLR_TABLE_BUILDER_HPP

#include "../cfg/cfg_algorithms.hpp"
#include "table.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <set>

namespace fsm::slr
{
namespace detail
{
struct throw_exception_t
{
};

struct prefer_shift_t
{
};

struct keep_first_t
{
};

struct keep_last_t
{
};
} // namespace detail

namespace collision_policy
{
inline constexpr detail::throw_exception_t throw_exception{};
inline constexpr detail::prefer_shift_t prefer_shift{};
inline constexpr detail::keep_first_t keep_first{};
inline constexpr detail::keep_last_t keep_last{};
} // namespace collision_policy

template <typename T_Symbol>
struct lr0_item
{
	cfg_rule<T_Symbol> rule;
	std::size_t dot = 0;

	[[nodiscard]] bool is_complete() const
	{
		return dot >= rule.rhs.size();
	}

	T_Symbol next_symbol() const
	{
		return rule.rhs[dot];
	}

	bool operator<(const lr0_item& other) const
	{
		if (dot != other.dot)
		{
			return dot < other.dot;
		}
		if (rule.lhs != other.rule.lhs)
		{
			return rule.lhs < other.rule.lhs;
		}

		return rule.rhs < other.rule.rhs;
	}

	bool operator==(const lr0_item& other) const
	{
		return dot == other.dot && rule.lhs == other.rule.lhs && rule.rhs == other.rule.rhs;
	}
};

template <typename T_Symbol, typename T_Compare = std::less<T_Symbol>>
class table_builder
{
	using grammar_t = basic_cfg<T_Symbol, T_Compare>;
	using item_t = lr0_item<T_Symbol>;
	using state_t = std::set<item_t>;
	using identity_symbol = std::type_identity_t<T_Symbol>;

public:
	using grammar_type = grammar_t;
	using table_type = table<T_Symbol, T_Compare>;
	using warning_callback_type = std::function<void(const std::string&)>;

	explicit table_builder(const grammar_t& grammar)
		: m_grammar(grammar)
	{
	}

	table_builder(
		const grammar_t& grammar,
		const identity_symbol& epsilon,
		const identity_symbol& end_marker)
		: m_grammar(grammar)
		, m_epsilon(epsilon)
		, m_eof(end_marker)
	{
	}

	table_builder& with_epsilon(const T_Symbol& epsilon)
	{
		m_epsilon = std::move(epsilon);
		return *this;
	}

	table_builder& with_end_marker(const T_Symbol& end_marker)
	{
		m_eof = std::move(end_marker);
		return *this;
	}

	table_builder& with_augmented_start(const T_Symbol& aug_start)
	{
		m_aug_start = std::move(aug_start);
		return *this;
	}

	table_builder& on_warning(warning_callback_type&& callback)
	{
		m_warning_callback = std::move(callback);
		return *this;
	}

	template <typename T_CollisionPolicy = detail::throw_exception_t>
	table_type build(T_CollisionPolicy policy = collision_policy::throw_exception) const
	{
		using tbl_state_t = typename table_type::state_type;
		using action_type = typename table_type::action_type;

		table_type result;
		result.set_end_marker(m_eof);

		auto first_sets = algorithms::compute_first(m_grammar, m_epsilon);
		auto follow_sets = algorithms::compute_follow(m_grammar, first_sets, m_epsilon, m_eof);

		std::vector<state_t> states;
		std::map<state_t, tbl_state_t> state_to_id;
		std::map<std::pair<tbl_state_t, T_Symbol>, tbl_state_t> transitions;

		state_t start_state = compute_closure({ { { m_aug_start, { m_grammar.start_symbol() } }, 0 } });
		states.push_back(start_state);
		state_to_id[start_state] = 0;

		std::queue<tbl_state_t> q;
		q.push(0);

		while (!q.empty())
		{
			tbl_state_t current_id = q.front();
			q.pop();
			const state_t current_state = states[current_id];

			std::set<T_Symbol, T_Compare> next_symbols;
			for (const auto& item : current_state)
			{
				if (!item.is_complete() && item.next_symbol() != m_epsilon)
				{
					next_symbols.insert(item.next_symbol());
				}
			}

			for (const auto& X : next_symbols)
			{
				state_t next_state = compute_goto(current_state, X);
				if (next_state.empty())
				{
					continue;
				}

				if (!state_to_id.contains(next_state))
				{
					auto new_id = static_cast<tbl_state_t>(states.size());
					states.push_back(next_state);
					state_to_id[next_state] = new_id;
					q.push(new_id);
				}

				transitions[{ current_id, X }] = state_to_id[next_state];
			}
		}

		auto try_add_action = [&](tbl_state_t state_id, const T_Symbol& terminal, const action_type& new_action) {
			const auto& existing = result.get_action(state_id, terminal);

			if (actions::is_error(existing))
			{
				result.add_action(state_id, terminal, new_action);

				return;
			}

			const bool is_sr = (actions::is_shift(existing) && actions::is_reduce(new_action))
				|| (actions::is_reduce(existing) && actions::is_shift(new_action));
			const bool is_rr = actions::is_reduce(existing) && actions::is_reduce(new_action);

			const std::string conflict_type = is_sr ? "Shift/Reduce" : (is_rr ? "Reduce/Reduce" : "Unknown");
			const std::string msg = "SLR(1) " + conflict_type + " conflict at state " + std::to_string(state_id);

			utility::overloaded{
				[&](detail::throw_exception_t) {
					throw std::runtime_error(msg);
				},
				[&](detail::prefer_shift_t) {
					if (is_sr)
					{
						if (actions::is_shift(new_action))
						{
							result.add_action(state_id, terminal, new_action);
						}
						call_warning_callback_if_set("[WARNING] " + msg + " -> Resolved: Prefer Shift.");
					}
					else if (is_rr)
					{
						call_warning_callback_if_set("[WARNING] " + msg + " -> Resolved: Keep First (R/R).");
					}
				},
				[&](detail::keep_first_t) {
					call_warning_callback_if_set("[WARNING] " + msg + " -> Resolved: Keep First.");
				},
				[&](detail::keep_last_t) {
					call_warning_callback_if_set("[WARNING] " + msg + " -> Resolved: Keep Last.");

					result.add_action(state_id, terminal, new_action);
				}
			}(policy);
		};

		for (tbl_state_t i = 0; i < states.size(); ++i)
		{
			for (const state_t& I = states[i]; const auto& item : I)
			{
				if (!item.is_complete())
				{ // A -> alpha . a beta (SHIFT)
					T_Symbol a = item.next_symbol();
					if (m_grammar.is_terminal(a) && transitions.contains({ i, a }))
					{
						try_add_action(i, a, action_shift<tbl_state_t>{ transitions.at({ i, a }) });
					}
					else if (m_grammar.is_non_terminal(a) && transitions.contains({ i, a }))
					{
						result.add_goto(i, a, transitions.at({ i, a }));
					}
				}
				else if (item.rule.lhs == m_aug_start)
				{ // S' -> S . (ACCEPT)
					try_add_action(i, m_eof, action_accept{});
				}
				else
				{ // A -> alpha . (REDUCE)
					for (const auto& a : follow_sets.at(item.rule.lhs))
					{
						try_add_action(i, a, action_reduce<T_Symbol>{ item.rule });
					}
				}
			}
		}

		return result;
	}

private:
	grammar_t m_grammar;

	T_Symbol m_epsilon;
	T_Symbol m_eof;
	T_Symbol m_aug_start;

	warning_callback_type m_warning_callback;

	state_t compute_closure(state_t I) const
	{
		bool changed = true;
		while (changed)
		{
			changed = false;
			std::vector<item_t> items_to_add;

			for (const auto& item : I)
			{
				if (!item.is_complete() && m_grammar.is_non_terminal(item.next_symbol()))
				{
					T_Symbol B = item.next_symbol();
					for (const auto& rule : m_grammar.rules())
					{
						if (rule.lhs == B)
						{
							std::size_t initial_dot = (rule.rhs.size() == 1 && rule.rhs[0] == m_epsilon) ? 1 : 0;
							if (item_t new_item{ rule, initial_dot }; !I.contains(new_item))
							{
								items_to_add.push_back(new_item);
							}
						}
					}
				}
			}

			for (const auto& new_item : items_to_add)
			{
				if (I.insert(new_item).second)
				{
					changed = true;
				}
			}
		}

		return I;
	}

	state_t compute_goto(const state_t& I, const T_Symbol& X) const
	{
		state_t J;
		for (const auto& item : I)
		{
			if (!item.is_complete() && item.next_symbol() == X)
			{
				J.insert({ item.rule, item.dot + 1 });
			}
		}

		return compute_closure(J);
	}

	void call_warning_callback_if_set(const std::string& msg) const
	{
		if (m_warning_callback)
		{
			m_warning_callback(msg);
		}
	}
};
} // namespace fsm::slr

#endif // FSM_SLR_TABLE_BUILDER_HPP
